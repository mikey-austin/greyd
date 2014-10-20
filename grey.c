/**
 * @file   grey.c
 * @brief  Implements the greylisting management interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "grey.h"
#include "greydb.h"
#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define T Grey_tuple
#define D Grey_data
#define G Greylister_T

#define GREY_DEFAULT_TL_NAME "greyd-greytrap"
#define GREY_DEFAULT_TL_MSG  "\"Your address %A has mailed to spamtraps here\\n\""

static void destroy_address(void *address);
static void sig_term_children(int sig);
static int scan_db(G greylister);
static int push_addr(List_T list, char *addr);
static int db_addr_state(DB_handle_T db, char *addr);
static void configure_greyd(G greylister);

G Grey_greylister = NULL;

extern G
Grey_setup(Config_T config)
{
    G greylister = NULL;
    Config_section_T section;

    if((greylister = malloc(sizeof(*greylister))) == NULL) {
        I_CRIT("Could not malloc greylister");
    }

    greylister->config     = config;
    greylister->trap_out   = NULL;
    greylister->grey_in    = NULL;
    greylister->grey_pid   = -1;
    greylister->reader_pid = -1;
    greylister->startup    = -1;

    section = Config_get_section(config, CONFIG_DEFAULT_SECTION);
    if(section) {
        greylister->traplist_name = Config_section_get_str(
            section, "traplist_name", GREY_DEFAULT_TL_NAME);

        greylister->traplist_msg = Config_section_get_str(
            section, "traplist_message", GREY_DEFAULT_TL_MSG);
    }

    greylister->whitelist = List_create(destroy_address);
    greylister->traplist  = List_create(destroy_address);

    return greylister;
}

extern void
Grey_start(G greylister, pid_t grey_pid, FILE *grey_in, FILE *trap_out)
{
    struct sigaction sa;

    // TODO: setup the database and drop privileges.

    greylister->grey_pid = grey_pid;
    greylister->startup  = time(NULL);
    greylister->grey_in  = grey_in;
    greylister->trap_out = trap_out;

    switch((greylister->reader_pid = fork()))
    {
    case -1:
        I_CRIT("Could not fork greyd reader");
        exit(1);

    case 0:
        /*
         * In child. This process has no access to the
         * firewall configuration handle, nor the traplist
         * configuration pipe to the main greyd process.
         */
        fclose(greylister->trap_out);

        // TODO: Close firewall handle here.

        // TODO: Set proc title to "(greyd db update)".

        if(Grey_start_reader(greylister) == -1) {
            I_CRIT("Greyd reader failed to start");
            exit(1);
        }
        _exit(0);
    }

    /*
     * In parent. This process has no access to the grey data being
     * sent from the main greyd process.
     */
    fclose(greylister->grey_in);

    /*
     * Set a global reference to the configured greylister state,
     * to make it available to signal handlers.
     */
    Grey_greylister = greylister;

    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sig_term_children;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // TODO: Set proc title "(greyd fw whitelist update)".

    Grey_start_scanner(greylister);

    exit(1); /* Not-reached. */
}

extern void
Grey_finish(G *greylister)
{
    pid_t grey_pid, reader_pid;

    if(*greylister == NULL)
        return;

    (*greylister)->config = NULL;

    List_destroy(&((*greylister)->whitelist));
    List_destroy(&((*greylister)->traplist));

    if((*greylister)->trap_out != NULL)
        fclose((*greylister)->trap_out);

    if((*greylister)->grey_in != NULL)
        fclose((*greylister)->grey_in);

    grey_pid   = (*greylister)->grey_pid;
    reader_pid = (*greylister)->reader_pid;

    free(*greylister);
    *greylister = NULL;
    Grey_greylister = NULL;

    if(grey_pid != -1)
        kill(grey_pid, SIGTERM);

    if(reader_pid != -1)
        kill(reader_pid, SIGTERM);
}

extern int
Grey_start_reader(G greylister)
{
    return 0;
}

extern void
Grey_start_scanner(G greylister)
{
    for(;;) {
        if(scan_db(greylister) == -1)
            I_WARN("db scan failed");
        sleep(GREY_DB_SCAN_INTERVAL);
    }
    /* Not reached. */
}

static int
scan_db(G greylister)
{
    DB_handle_T db;
    DB_itr_T itr;
    struct DB_key key, wkey;
    struct DB_val val, wval;
    struct Grey_tuple gt;
    struct Grey_data gd;
    time_t now = time(NULL);
    int ret = 0, state;

    db = DB_open(greylister->config, 0);
    itr = DB_get_itr(db);

    while(DB_itr_next(itr, &key, &val) == GREYDB_FOUND) {
        if(val.type != DB_VAL_GREY)
            continue;

        gd = val.data.gd;
        if(gd.expire <= now && gd.pcount != -2) {
            /*
             * This non-spamtrap entry has expired.
             */
            if(DB_itr_del_curr(itr) != GREYDB_OK) {
                ret = -1;
                goto cleanup;
            }
            continue;
        }
        else if(gd.pcount == -1 && key.type == DB_KEY_IP) {
            /*
             * This is a greytrap hit.
             */
            if((push_addr(greylister->traplist, key.data.s) == -1)
               && DB_itr_del_curr(itr) != GREYDB_OK)
            {
                ret = -1;
                goto cleanup;
            }
        }
        else if(gd.pcount >= 0 && gd.pass <= now) {
            /*
             * If not already trapped, add the address to the whitelist
             * and add an address-keyed entry to the database.
             */

            if(key.type == DB_KEY_TUPLE) {
                gt = key.data.gt;
                if((state = db_addr_state(db, gt.ip)) == 1)
                    continue;

                if((push_addr(greylister->whitelist, gt.ip) == -1)
                   && DB_itr_del_curr(itr) != GREYDB_OK)
                {
                    ret = -1;
                    goto cleanup;
                }

                /* Re-add entry, keyed only by IP address. */
                memset(&wkey, 0, sizeof(wkey));
                wkey.type = DB_KEY_IP;
                wkey.data.s = gt.ip;

                memset(&wval, 0, sizeof(wval));
                wval.type = DB_VAL_GREY;
                gd.expire = now + GREY_WHITEEXP;
                wval.data.gd = gd;

                if(DB_put(db, &wkey, &wval) != GREYDB_OK) {
                    ret = -1;
                    goto cleanup;
                }

                I_DEBUG("whitelisting %s", gt.ip);
            }
            else if(key.type == DB_KEY_IP) {
                /*
                 * This must be a whitelist entry.
                 */
                if((push_addr(greylister->whitelist, key.data.s) == -1)
                   && DB_itr_del_curr(itr) != GREYDB_OK)
                {
                    ret = -1;
                    goto cleanup;
                }
            }
        }
    }

    configure_greyd(greylister);

    // TODO: Send whitelist IPs to firewall

    List_remove_all(greylister->whitelist);
    List_remove_all(greylister->traplist);

cleanup:
    DB_close_itr(&itr);
    DB_close(&db);
    return ret;
}

static void
configure_greyd(G greylister)
{
    struct List_entry_T *entry;

    if(List_size(greylister->traplist) > 0) {
        fprintf(greylister->trap_out, "%s;%s;",
                greylister->traplist_name,
                greylister->traplist_msg);

        LIST_FOREACH(greylister->traplist, entry) {
            fprintf(greylister->trap_out, "%s/32;",
                    List_entry_value(entry));
        }
    }
    fprintf(greylister->trap_out, "\n");

    if(fflush(greylister->trap_out) == EOF)
        I_DEBUG("configure_greyd: fflush failed");
}

/**
 * Check the state of the supplied address via the opened
 * database handle.
 *
 * @return -1 on error
 * @return 0 when not found
 * @return 1 if greytrapped
 * @return 2 if whitelisted
 */
static int
db_addr_state(DB_handle_T db, char *addr)
{
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;
    int ret;

    key.type = DB_KEY_IP;
    key.data.s = addr;
    ret = DB_get(db, &key, &val);

    switch(ret) {
    case GREYDB_NOT_FOUND:
        return 0;

    case GREYDB_FOUND:
        gd = val.data.gd;
        return (gd.pcount == -1 ? 1 : 2);

    default:
        return -1;
    }
}

/**
 * Validate address as a valid IP address and add to the
 * the specified list, returning 0. If the validation fails,
 * a -1 is returned.
 */
static int
push_addr(List_T list, char *addr)
{
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;  /* Dummy. */
    hints.ai_protocol = IPPROTO_UDP; /* Dummy. */
    hints.ai_flags = AI_NUMERICHOST;

    if(getaddrinfo(addr, NULL, &hints, &res) == 0) {
        freeaddrinfo(res);
        List_insert_after(list, strdup(addr));
    }
    else {
        return -1;
    }

    return 0;
}

static void
destroy_address(void *address)
{
    char *to_destroy = (char *) address;

    if(to_destroy != NULL)
        free(to_destroy);
}

static void
sig_term_children(int sig)
{
    if(Grey_greylister) {
        if(Grey_greylister->grey_pid != -1)
            kill(Grey_greylister->grey_pid, SIGTERM);

        if(Grey_greylister->reader_pid != -1)
            kill(Grey_greylister->reader_pid, SIGTERM);
    }

    _exit(1);
}

#undef T
#undef D
#undef G
