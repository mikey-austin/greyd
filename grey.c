/**
 * @file   grey.c
 * @brief  Implements the greylisting management interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "grey.h"
#include "greydb.h"
#include "config.h"
#include "config_lexer.h"
#include "config_parser.h"
#include "list.h"
#include "ip.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define T Grey_tuple
#define D Grey_data
#define G Greylister_T

#define GREY_DEFAULT_TL_NAME "greyd-greytrap"
#define GREY_DEFAULT_TL_MSG  "\"Your address %A has mailed to spamtraps here\\n\""

static void destroy_address(void *);
static void sig_term_children(int);
static int scan_db(G);
static int push_addr(List_T, char *);
static void configure_greyd(G);
static void process_message(G, Config_T);
static void process_grey(G, struct T *, int, char *);
static void process_non_grey(G, int, char *, char *, char *);
static int trap_check(DB_handle_T, char *);
static int db_addr_state(DB_handle_T, char *);

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

    /*
     * Gather configuration from the grey section.
     */
    section = Config_get_section(config, "grey");
    if(section) {
        greylister->traplist_name = Config_section_get_str(
            section, "traplist_name", GREY_DEFAULT_TL_NAME);

        greylister->traplist_msg = Config_section_get_str(
            section, "traplist_message", GREY_DEFAULT_TL_MSG);

        greylister->grey_exp = Config_section_get_int(
            section, "grey_expiry", GREY_GREYEXP);

        greylister->white_exp = Config_section_get_int(
            section, "white_expiry", GREY_WHITEEXP);

        greylister->trap_exp = Config_section_get_int(
            section, "trap_expiry", GREY_TRAPEXP);
    }

    section = Config_get_section(config, CONFIG_DEFAULT_SECTION);
    if(section) {
        greylister->low_prio_mx_ip = Config_section_get_str(
            section, "low_prio_mx_ip", NULL);

        greylister->sync_send = Config_section_get_int(
            section, "sync", 0);
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
    Lexer_source_T source;
    Lexer_T lexer;
    Config_parser_T parser;
    Config_T message;
    int ret, fd;

    fd = fileno(greylister->grey_in);
    if(fd == -1) {
        I_WARN("No greylist pipe stream");
        return 0;
    }

    source = Lexer_source_create_from_fd(fd);
    lexer = Config_lexer_create(source);
    parser = Config_parser_create(lexer);

    for(;;) {
        message = Config_create();

        ret = Config_parser_start(parser, message);
        switch(ret) {
        case CONFIG_PARSER_OK:
            process_message(greylister, message);
            break;

        case CONFIG_PARSER_ERR:
            goto cleanup;
        }

        Config_destroy(&message);
    }

cleanup:
    Config_destroy(&message);
    Config_parser_destroy(&parser);

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
                gd.expire = now + greylister->white_exp;
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
    char *ip;
    int first = 1;
    short af;

    if(List_size(greylister->traplist) > 0) {
        fprintf(greylister->trap_out,
                "name=\"%s\"\nmessage=\"%s\"\nips=[",
                greylister->traplist_name,
                greylister->traplist_msg);

        LIST_FOREACH(greylister->traplist, entry) {
            ip = List_entry_value(entry);
            af = IP_check_addr(ip);

            if(!first)
                fprintf(greylister->trap_out, ",");

            fprintf(greylister->trap_out, "\"%s/%d\"", ip,
                    (af == AF_INET ? 32 : 128));
            first = 0;
        }
    }
    fprintf(greylister->trap_out, "]\n%%");

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
 * the specified list.
 *
 * @return 0  Address validated and added to list.
 * @return -1 Address was not added to list.
 */
static int
push_addr(List_T list, char *addr)
{
    if(IP_check_addr(addr) != -1) {
        List_insert_after(list, strdup(addr));
        return 0;
    }

    return -1;
}

/**
 * Check if the specified to address matches either a trap
 * address, or does not satisfy the allowed domains (if specified).
 *
 * @return 0  Address is a spamtrap.
 * @return 1  Address doesn't match a known spamtrap.
 * @return -1 An error occured.
 */
static int
trap_check(DB_handle_T db, char *to)
{
    struct DB_key key;
    struct DB_val val;
    int ret;

    key.type = DB_KEY_MAIL;
    key.data.s = to;
    ret = DB_get(db, &key, &val);

    switch(ret) {
    case GREYDB_FOUND:
        return 0;

    case GREYDB_NOT_FOUND:
        return 1;

    default:
        return -1;
    }
}

static void
process_grey(G greylister, struct T *gt, int sync, char *dst_ip)
{
    DB_handle_T db;
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;
    time_t now, expire;
    int spamtrap;

    now = time(NULL);
    db = DB_open(greylister->config, 0);

    switch(trap_check(db, gt->to)) {
    case 1:
        /* Do not trap. */
        spamtrap = 0;
        expire = greylister->grey_exp;
        key.type = DB_KEY_TUPLE;
        key.data.gt = *gt;
        break;

    case 0:
        /* Trap address. */
        spamtrap = 1;
        expire = greylister->trap_exp;
        key.type = DB_KEY_IP;
        key.data.s = gt->ip;
        break;

    default:
        goto cleanup;
        break;
    }

    switch(DB_get(db, &key, &val)) {
    case GREYDB_NOT_FOUND:
        /*
         * We have a new entry.
         */
        if(sync && greylister->low_prio_mx_ip
           && (strcmp(dst_ip, greylister->low_prio_mx_ip) == 0)
           && ((greylister->startup + 60) < now))
        {
            /*
             * We haven't seen a greylist entry for this tuple,
			 * and yet the connection was to a low priority MX
			 * which we know can't be hit first if the client
			 * is adhering to the RFC's.
             */
            spamtrap = 1;
            expire = greylister->trap_exp;
            key.type = DB_KEY_IP;
            key.data.s = gt->ip;
            I_DEBUG("trapping %s for trying %s first for tuple (%s, %s, %s, %s)",
                    gt->ip, greylister->low_prio_mx_ip,
                    gt->ip, gt->helo, gt->from, gt->to);
        }

        gd.first = now;
        gd.bcount = 1;
        gd.pcount = (spamtrap ? -1 : 0);
        gd.pass = now + expire;
        gd.expire = now + expire;
        val.type = DB_VAL_GREY;
        val.data.gd = gd;

        switch(DB_put(db, &key, &val)) {
        case GREYDB_OK:
            I_DEBUG("new %sentry %s from %s to %s, helo %s",
                    (spamtrap ? "greytrap " : ""), gt->ip,
                    gt->from, gt->to, gt->helo );
            break;

        default:
            goto cleanup;
            break;
        }
        break;

    case GREYDB_FOUND:
        /*
         * We have a previously seen entry.
         */
        gd = val.data.gd;
        gd.bcount++;
        gd.pcount = (spamtrap ? -1 : 0);
        val.data.gd = gd;

        if(DB_put(db, &key, &val) == GREYDB_OK) {
            I_DEBUG("updated %sentry %s from %s to %s, helo %s",
                    (spamtrap ? "greytrap " : ""), gt->ip,
                    gt->from, gt->to, gt->helo );
        }
        else {
            goto cleanup;
        }
        break;

    default:
        goto cleanup;
        break;
    }

    /*
     * Entry successfully updated, send out sync message.
     */
    if(greylister->sync_send && sync) {
        if(spamtrap) {
            I_DEBUG("sync_trap %s", gt->ip);
            // TODO: sync_trapped(now, now + expire, gt->ip);
        }
        else {
            // TODO: sync_update(now, gt->helo, gt->ip, gt->from, gt->to);
        }
    }

cleanup:
    DB_close(&db);
}

static void
process_non_grey(G greylister, int spamtrap, char *ip, char *source, char *expires)
{
    DB_handle_T db;
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;
    time_t now;
    long expire;
    char *end;

    now = time(NULL);

	/* Expiry times have to be in the future. */
    errno = 0;
    expire = strtol(expires, &end, 10);
    if(expire == 0 || expires[0] == '\0' || *end != '\0'
       || (errno == ERANGE && (expire == LONG_MAX || expire == LONG_MIN)))
    {
        I_WARN("could not parse expires %s", expires);
        return;
    }

    key.type = DB_KEY_IP;
    key.data.s = ip;

    db = DB_open(greylister->config, 0);
    switch(DB_get(db, &key, &val)) {
    case GREYDB_NOT_FOUND:
        /*
         * This is a new entry.
         */
        memset(&gd, 0, sizeof(gd));
        gd.first = now;
        gd.pcount = (spamtrap ? -1 : 0);
        gd.expire = expire;
        val.type = DB_VAL_GREY;
        val.data.gd = gd;

        if(DB_put(db, &key, &val) == GREYDB_OK) {
            I_DEBUG("new %s from %s for %s, expires %s",
                    (spamtrap ? "TRAP" : "WHITE"), source, ip,
                    expires);
        }
        break;

    case GREYDB_FOUND:
        /*
         * This is an existing entry.
         */
        if(spamtrap) {
            val.data.gd.pcount = -1;
            val.data.gd.bcount++;
        }
        else {
            val.data.gd.pcount++;
        }

        if(DB_put(db, &key, &val) == GREYDB_OK) {
            I_DEBUG("updated %s", ip);
        }
        break;

    default:
        goto cleanup;
        break;
    }

cleanup:
    DB_close(&db);
}

static void
process_message(G greylister, Config_T message)
{
    Config_section_T section;
    int type, sync;
    struct Grey_tuple gt;
    char *dst_ip, *ip, *source, *expires;

    section = Config_get_section(message, CONFIG_DEFAULT_SECTION);
    type    = Config_section_get_int(section, "type", -1);
    dst_ip  = Config_section_get_str(section, "dst_ip", NULL);

    /*
     * If this message isn't a SYNC message from another greyd,
     * sync future operations.
     */
    sync = Config_section_get_int(section, "sync", 1);

    switch(type) {
    case GREY_MSG_GREY:
        gt.ip   = Config_section_get_str(section, "ip", NULL);
        gt.helo = Config_section_get_str(section, "helo", NULL);
        gt.from = Config_section_get_str(section, "from", NULL);
        gt.to   = Config_section_get_str(section, "to", NULL);
        if(gt.ip && gt.helo && gt.from && gt.to)
            process_grey(greylister, &gt, sync, dst_ip);
        break;

    case GREY_MSG_TRAP:
    case GREY_MSG_WHITE:
        ip      = Config_section_get_str(section, "ip", NULL);
        source  = Config_section_get_str(section, "source", NULL);
        expires = Config_section_get_str(section, "expires", NULL);
        if(ip && source && expires)
            process_non_grey(greylister, (type == GREY_MSG_TRAP ? 1 : 0),
                             ip, source, expires);
        break;

    default:
        I_WARN("Unknown greylist message type");
        return;
    }
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
