/**
 * @file   grey.c
 * @brief  Implements the greylisting management interface.
 * @author Mikey Austin
 * @date   2014
 */

#define _GNU_SOURCE

#include "constants.h"
#include "failures.h"
#include "grey.h"
#include "greyd.h"
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
#include <grp.h>

#define GREY_TRAP_NAME       "greyd-greytrap"
#define GREY_TRAP_MSG        "Your address %A has mailed to spamtraps here"
#define GREY_WHITE_NAME      "greyd-whitelist"
#define GREY_WHITE_NAME_IPV6 "greyd-whitelist-ipv6"

static void destroy_address(void *);
static void sig_term_children(int);
static int push_addr(List_T, char *);
static void process_message(Greylister_T, Config_T);
static void process_grey(Greylister_T, struct Grey_tuple *, int, char *);
static void process_non_grey(Greylister_T, int, char *, char *, char *);
static int trap_check(DB_handle_T, char *);
static int db_addr_state(DB_handle_T, char *);

Greylister_T Grey_greylister = NULL;

extern Greylister_T
Grey_setup(Config_T config)
{
    Greylister_T greylister = NULL;

    if((greylister = malloc(sizeof(*greylister))) == NULL) {
        i_critical("Could not malloc greylister");
    }

    greylister->config     = config;
    greylister->trap_out   = NULL;
    greylister->grey_in    = NULL;
    greylister->grey_pid   = -1;
    greylister->reader_pid = -1;
    greylister->startup    = -1;

    if((greylister->fw_handle = FW_open(config)) == NULL) {
        i_critical("Could not create firewall handle");
    }

    /*
     * Gather configuration from the grey section.
     */
    greylister->traplist_name = Config_get_str(
        config, "traplist_name", "grey", GREY_TRAP_NAME);

    greylister->traplist_msg = Config_get_str(
        config, "traplist_message", "grey", GREY_TRAP_MSG);

    greylister->whitelist_name = Config_get_str(
        config, "whitelist_name", "grey", GREY_WHITE_NAME);

    greylister->whitelist_name_ipv6 = Config_get_str(
        config, "whitelist_name_ipv6", "grey", GREY_WHITE_NAME_IPV6);

    greylister->grey_exp = Config_get_int(
        config, "grey_expiry", "grey", GREY_GREYEXP);

    greylister->white_exp = Config_get_int(
        config, "white_expiry", "grey", GREY_WHITEEXP);

    greylister->trap_exp = Config_get_int(
        config, "trap_expiry", "grey", GREY_TRAPEXP);

    greylister->pass_time = Config_get_int(
        config, "pass_time", "grey", GREY_PASSTIME);

    greylister->low_prio_mx = Config_get_str(
        config, "low_prio_mx",NULL , NULL);

    greylister->sync_send = Config_get_int(
        config, "sync", NULL, 0);

    greylister->whitelist      = List_create(destroy_address);
    greylister->whitelist_ipv6 = List_create(destroy_address);
    greylister->traplist       = List_create(destroy_address);

    return greylister;
}

extern void
Grey_start(Greylister_T greylister, pid_t grey_pid, FILE *grey_in, FILE *trap_out)
{
    struct sigaction sa;
    char *db_user;
    struct passwd *db_pw;

    db_user = Config_get_str(greylister->config, "user", "grey",
                             GREYD_DB_USER);
    if((db_pw = getpwnam(db_user)) == NULL)
        i_critical("no such user %s", db_user);

    if(db_pw && Config_get_int(greylister->config, "drop_privs", NULL, 1)) {
        if(setgroups(1, &db_pw->pw_gid)
           || setresgid(db_pw->pw_gid, db_pw->pw_gid, db_pw->pw_gid)
           || setresuid(db_pw->pw_uid, db_pw->pw_uid, db_pw->pw_uid))
        {
            i_critical("failed to drop privileges");
        }
    }

    greylister->grey_pid = grey_pid;
    greylister->startup  = time(NULL);
    greylister->grey_in  = grey_in;
    greylister->trap_out = trap_out;

    switch((greylister->reader_pid = fork()))
    {
    case -1:
        i_critical("Could not fork greyd reader");
        exit(1);

    case 0:
        /*
         * In child. This process has no access to the
         * firewall configuration handle, nor the traplist
         * configuration pipe to the main greyd process.
         */
        fclose(greylister->trap_out);
        FW_close(&(greylister->fw_handle));

        /* TODO: Set proc title to "(greyd db update)". */

        if(Grey_start_reader(greylister) == -1) {
            i_critical("Greyd reader failed to start");
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

    /* TODO: Set proc title "(greyd fw whitelist update)". */

    Grey_start_scanner(greylister);

    exit(1); /* Not-reached. */
}

extern void
Grey_finish(Greylister_T *greylister)
{
    pid_t grey_pid, reader_pid;

    if(*greylister == NULL)
        return;

    (*greylister)->config = NULL;

    FW_close(&((*greylister)->fw_handle));
    List_destroy(&((*greylister)->whitelist));
    List_destroy(&((*greylister)->whitelist_ipv6));
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
Grey_start_reader(Greylister_T greylister)
{
    Lexer_source_T source;
    Lexer_T lexer;
    Config_parser_T parser;
    Config_T message;
    int ret, fd;

    fd = fileno(greylister->grey_in);
    if(fd == -1) {
        i_warning("No greylist pipe stream");
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
Grey_start_scanner(Greylister_T greylister)
{
    for(;;) {
        if(Grey_scan_db(greylister) == -1)
            i_warning("db scan failed");
        sleep(GREY_DB_SCAN_INTERVAL);
    }
    /* Not reached. */
}

extern int
Grey_scan_db(Greylister_T greylister)
{
    DB_handle_T db;
    DB_itr_T itr;
    struct DB_key key, wkey;
    struct DB_val val, wval;
    struct Grey_tuple gt;
    struct Grey_data gd;
    List_T list;
    time_t now = time(NULL);
    int ret = 0;

    db = DB_init(greylister->config);
    DB_open(db, 0);
    DB_start_txn(db);
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
            else {
                i_debug("deleting expired %sentry %s",
                        (key.type == DB_KEY_IP
                         ? (gd.pcount >= 0 ? "white " : "greytrap ")
                         : "grey "),
                        (key.type == DB_KEY_IP ? key.data.s : key.data.gt.ip));
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
                switch(db_addr_state(db, gt.ip)) {
                case 1:
                    /* Ignore trapped entries. */
                    continue;

                case -1:
                    ret = -1;
                    goto cleanup;
                }

                list = (IP_check_addr(key.data.s) == AF_INET6
                        ? greylister->whitelist_ipv6
                        : greylister->whitelist);

                if((push_addr(list, gt.ip) == -1)
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

                i_debug("whitelisting %s", gt.ip);
            }
            else if(key.type == DB_KEY_IP) {
                /*
                 * This must be a whitelist entry.
                 */
                list = (IP_check_addr(key.data.s) == AF_INET6
                        ? greylister->whitelist_ipv6
                        : greylister->whitelist);

                if((push_addr(list, key.data.s) == -1)
                   && DB_itr_del_curr(itr) != GREYDB_OK)
                {
                    ret = -1;
                    goto cleanup;
                }
            }
        }
    }

    DB_close_itr(&itr);
    DB_commit_txn(db);

    Greyd_send_config(greylister->trap_out,
                      greylister->traplist_name,
                      greylister->traplist_msg,
                      greylister->traplist);

    FW_replace(greylister->fw_handle, greylister->whitelist_name,
               greylister->whitelist, AF_INET);

    if(Config_get_int(greylister->config, "enable_ipv6", NULL,
                      IPV6_ENABLED))
    {
        FW_replace(greylister->fw_handle, greylister->whitelist_name_ipv6,
                   greylister->whitelist_ipv6, AF_INET6);
    }

    List_remove_all(greylister->whitelist);
    List_remove_all(greylister->whitelist_ipv6);
    List_remove_all(greylister->traplist);

cleanup:
    if(ret < 0)
        DB_rollback_txn(db);
    DB_close(&db);

    return ret;
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

    DB_start_txn(db);
    ret = DB_get(db, &key, &val);
    switch(ret) {
    case GREYDB_FOUND:
        DB_commit_txn(db);
        return 0;

    case GREYDB_NOT_FOUND:
        DB_commit_txn(db);
        return 1;

    default:
        DB_rollback_txn(db);
        return -1;
    }
}

static void
process_grey(Greylister_T greylister, struct Grey_tuple *gt, int sync, char *dst_ip)
{
    DB_handle_T db;
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;
    time_t now, expire, pass_time;
    int spamtrap;

    now = time(NULL);
    db = DB_init(greylister->config);
    DB_open(db, 0);

    switch(trap_check(db, gt->to)) {
    case 1:
        /* Do not trap. */
        spamtrap = 0;
        expire = greylister->grey_exp;
        pass_time = greylister->pass_time;
        key.type = DB_KEY_TUPLE;
        key.data.gt = *gt;
        break;

    case 0:
        /* Trap address. */
        spamtrap = 1;
        expire = greylister->trap_exp;
        pass_time = greylister->trap_exp;
        key.type = DB_KEY_IP;
        key.data.s = gt->ip;
        break;

    default:
        goto rollback;
        break;
    }

    DB_start_txn(db);
    switch(DB_get(db, &key, &val)) {
    case GREYDB_NOT_FOUND:
        /*
         * We have a new entry.
         */
        if(sync && greylister->low_prio_mx
           && (strcmp(dst_ip, greylister->low_prio_mx) == 0)
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
            i_debug("trapping %s for trying %s first for tuple (%s, %s, %s, %s)",
                    gt->ip, greylister->low_prio_mx,
                    gt->ip, gt->helo, gt->from, gt->to);
        }

        gd.first = now;
        gd.bcount = 1;
        gd.pcount = (spamtrap ? -1 : 0);
        gd.pass = now + pass_time;
        gd.expire = now + expire;
        val.type = DB_VAL_GREY;
        val.data.gd = gd;

        switch(DB_put(db, &key, &val)) {
        case GREYDB_OK:
            i_debug("new %sentry %s from %s to %s, helo %s",
                    (spamtrap ? "greytrap " : ""), gt->ip,
                    gt->from, gt->to, gt->helo );
            break;

        default:
            goto rollback;
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
        if((gd.first + greylister->pass_time) < now)
            gd.pass = now;
        val.data.gd = gd;

        if(DB_put(db, &key, &val) == GREYDB_OK) {
            i_debug("updated %sentry %s from %s to %s, helo %s",
                    (spamtrap ? "greytrap " : ""), gt->ip,
                    gt->from, gt->to, gt->helo );
        }
        else {
            goto rollback;
        }
        break;

    default:
        goto rollback;
        break;
    }

    /*
     * Entry successfully updated, send out sync message.
     */
    if(greylister->sync_send && sync) {
        if(spamtrap) {
            i_debug("sync_trap %s", gt->ip);
            /* TODO: sync_trapped(now, now + expire, gt->ip); */
        }
        else {
            /* TODO: sync_update(now, gt->helo, gt->ip, gt->from, gt->to); */
        }
    }

    DB_commit_txn(db);
    DB_close(&db);
    return;

rollback:
    DB_rollback_txn(db);
    DB_close(&db);
}

static void
process_non_grey(Greylister_T greylister, int spamtrap, char *ip, char *source, char *expires)
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
        i_warning("could not parse expires %s", expires);
        return;
    }

    key.type = DB_KEY_IP;
    key.data.s = ip;

    db = DB_init(greylister->config);
    DB_open(db, 0);
    DB_start_txn(db);

    switch(DB_get(db, &key, &val)) {
    case GREYDB_NOT_FOUND:
        /*
         * This is a new entry.
         */
        memset(&gd, 0, sizeof(gd));
        gd.first = now;
        gd.pcount = (spamtrap ? -1 : 0);
        gd.pass = (spamtrap ? expire : now);
        gd.expire = expire;
        val.type = DB_VAL_GREY;
        val.data.gd = gd;

        if(DB_put(db, &key, &val) == GREYDB_OK) {
            i_debug("new %s from %s for %s, expires %s",
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
            i_debug("updated %s", ip);
        }
        break;

    default:
        goto rollback;
        break;
    }

    DB_commit_txn(db);
    DB_close(&db);
    return;

rollback:
    DB_rollback_txn(db);
    DB_close(&db);
}

static void
process_message(Greylister_T greylister, Config_T message)
{
    int type, sync;
    struct Grey_tuple gt;
    char *dst_ip, *ip, *source, *expires;

    type   = Config_get_int(message, "type", NULL, -1);
    dst_ip = Config_get_str(message, "dst_ip", NULL, NULL);

    /*
     * If this message isn't a SYNC message from another greyd,
     * sync future operations.
     */
    sync = Config_get_int(message, "sync", NULL, 1);

    switch(type) {
    case GREY_MSG_GREY:
        gt.ip   = Config_get_str(message, "ip", NULL, NULL);
        gt.helo = Config_get_str(message, "helo", NULL, NULL);
        gt.from = Config_get_str(message, "from", NULL, NULL);
        gt.to   = Config_get_str(message, "to", NULL, NULL);
        if(gt.ip && gt.helo && gt.from && gt.to)
            process_grey(greylister, &gt, sync, dst_ip);
        break;

    case GREY_MSG_TRAP:
    case GREY_MSG_WHITE:
        ip      = Config_get_str(message, "ip", NULL, NULL);
        source  = Config_get_str(message, "source", NULL, NULL);
        expires = Config_get_str(message, "expires", NULL, NULL);
        if(ip && source && expires)
            process_non_grey(greylister, (type == GREY_MSG_TRAP ? 1 : 0),
                             ip, source, expires);
        break;

    default:
        i_warning("Unknown greylist message type %d", type);
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
