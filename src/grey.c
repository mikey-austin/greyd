/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file   grey.c
 * @brief  Implements the greylisting management interface.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <grp.h>
#include <limits.h>
#include <ctype.h>

#include "constants.h"
#include "utils.h"
#include "failures.h"
#include "grey.h"
#include "sync.h"
#include "greyd.h"
#include "greydb.h"
#include "greyd_config.h"
#include "config_lexer.h"
#include "config_parser.h"
#include "list.h"
#include "ip.h"
#include "plugin.h"

#define GREY_TRAP_NAME       "greyd-greytrap"
#define GREY_TRAP_MSG        "Your address %A has mailed to spamtraps here"
#define GREY_WHITE_NAME      "greyd-whitelist"
#define GREY_WHITE_NAME_IPV6 "greyd-whitelist-ipv6"

static void destroy_address(void *);
static void drop_grey_privs(Greylister_T, struct passwd *);
static void shutdown_greyd(int);
static int process_message(Greylister_T, Config_T);
static void process_grey(Greylister_T, struct Grey_tuple *, int, char *);
static void process_non_grey(Greylister_T, int, char *, char *, char *, int, int);
static int trap_check(Greylister_T, struct Grey_tuple *);
static void update_firewall(Greylister_T, int);

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
    greylister->syncer     = NULL;
    greylister->shutdown   = 0;

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
        config, "low_prio_mx", NULL, "grey");

    greylister->traplist = List_create(destroy_address);
    greylister->whitelist = List_create(destroy_address);
    greylister->whitelist_ipv6 = List_create(destroy_address);
    greylister->domains = List_create(destroy_address);

    Grey_load_domains(greylister);

    return greylister;
}

extern void
Grey_start(Greylister_T greylister, pid_t grey_pid, FILE *grey_in,
           FILE *trap_out, FILE *fw_out)
{
    char *pidfile, *db_user;
    struct passwd *db_pw;
    struct sigaction sa;
    List_T hosts;

    memset(&sa, 0, sizeof(sa));
    greylister->grey_pid = grey_pid;
    greylister->startup  = time(NULL);
    greylister->grey_in  = grey_in;
    greylister->trap_out = trap_out;
    greylister->fw_out = fw_out;

    /*
     * Set a global reference to the configured greylister state,
     * to make it available to signal handlers.
     */
    Grey_greylister = greylister;

    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = shutdown_greyd;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    db_user = Config_get_str(greylister->config, "user", "grey",
                             GREYD_DB_USER);
    if((db_pw = getpwnam(db_user)) == NULL)
        i_critical("no such user %s", db_user);

    switch((greylister->reader_pid = fork()))
    {
    case -1:
        i_critical("Could not fork greyd reader");
        exit(1);

    case 0:
        /*
         * In child. Obtain a fresh database handle.
         */
        if((greylister->db_handle = DB_init(greylister->config)) == NULL)
            i_critical("Could not create db handle");

        /*
         * Setup the sync engine for sending only.
         */
        Config_delete(greylister->config, "bind_address", "sync");

        if((hosts = Config_get_list(greylister->config, "hosts", "sync"))
           && List_size(hosts) > 0
           && (greylister->syncer = Sync_init(greylister->config))
           && Sync_start(greylister->syncer) == -1)
        {
            i_warning("could not start sync engine");
            Sync_stop(&greylister->syncer);
        }

        drop_grey_privs(greylister, db_pw);

        /*
         * This process has no access to the firewall configuration
         * handle, nor the traplist configuration pipe to the main
         * greyd process.
         */
        fclose(greylister->trap_out);
        fclose(greylister->fw_out);
        greylister->trap_out = NULL;
        greylister->fw_out = NULL;

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
    if((greylister->db_handle = DB_init(greylister->config)) == NULL) {
        i_critical("Could not create db handle");
    }

    fclose(greylister->grey_in);
    greylister->grey_in = NULL;

    drop_grey_privs(greylister, db_pw);

    /* TODO: Set proc title "(greyd fw whitelist update)". */

    Grey_start_scanner(greylister);
    /* Not reached. */

error:
    Grey_finish(&greylister);
    exit(1);
}

extern void
Grey_finish(Greylister_T *greylister)
{
    if(*greylister == NULL)
        return;

    (*greylister)->config = NULL;

    DB_close(&((*greylister)->db_handle));
    List_destroy(&((*greylister)->whitelist));
    List_destroy(&((*greylister)->whitelist_ipv6));
    List_destroy(&((*greylister)->traplist));
    List_destroy(&((*greylister)->domains));

    if((*greylister)->trap_out != NULL)
        fclose((*greylister)->trap_out);

    if((*greylister)->grey_in != NULL)
        fclose((*greylister)->grey_in);

    if((*greylister)->fw_out != NULL)
        fclose((*greylister)->fw_out);

    free(*greylister);
    *greylister = NULL;
    Grey_greylister = NULL;
}

extern int
Grey_start_reader(Greylister_T greylister)
{
    Lexer_source_T source;
    Lexer_T lexer;
    Config_parser_T parser;
    Config_T message = NULL;
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
        if(greylister->shutdown) {
            i_debug("stopping grey reader");
            goto cleanup;
        }

        message = Config_create();
        ret = Config_parser_start(parser, message);
        switch(ret) {
        case CONFIG_PARSER_OK:
            if(process_message(greylister, message) == -1)
                goto cleanup;
            break;

        case CONFIG_PARSER_ERR:
            goto cleanup;
        }

        Config_destroy(&message);

        /*
         * Check if an error occured on the grey_in end of the pipe, as
         * we can't distinguish between a legitimate EOF and an error.
         */
        if(Lexer_source_error(source)) {
            i_debug("error detected on grey_in: %s", strerror(errno));
            goto cleanup;
        }
    }

cleanup:
    if(message != NULL)
        Config_destroy(&message);
    Config_parser_destroy(&parser);
    Grey_finish(&greylister);

    return 0;
}

extern void
Grey_start_scanner(Greylister_T greylister)
{
    for(;;) {
        if(greylister->shutdown)
            break;

        if(Grey_scan_db(greylister) == -1)
            i_warning("db scan failed");

        sleep(GREY_DB_SCAN_INTERVAL);
    }

    i_info("exiting");
    if(Grey_greylister->grey_pid != -1)
        kill(Grey_greylister->grey_pid, SIGTERM);

    if(Grey_greylister->reader_pid != -1)
        kill(Grey_greylister->reader_pid, SIGTERM);

    Grey_finish(&greylister);

    exit(0);
}

extern int
Grey_scan_db(Greylister_T greylister)
{
    DB_handle_T db = greylister->db_handle;
    time_t now = time(NULL);
    int ret = 0;

    DB_open(db, 0);
    DB_start_txn(db);

    if(DB_scan(db, &now, greylister->whitelist, greylister->whitelist_ipv6,
               greylister->traplist, &greylister->white_exp) != GREYDB_OK)
    {
        ret = -1;
        goto cleanup;
    }
    DB_commit_txn(db);

    Greyd_send_config(greylister->trap_out,
                      greylister->traplist_name,
                      greylister->traplist_msg,
                      greylister->traplist);

    update_firewall(greylister, AF_INET);
    if(Config_get_int(greylister->config, "enable_ipv6", NULL, IPV6_ENABLED))
        update_firewall(greylister, AF_INET6);

cleanup:
    List_remove_all(greylister->whitelist);
    List_remove_all(greylister->whitelist_ipv6);
    List_remove_all(greylister->traplist);

    if(ret < 0)
        DB_rollback_txn(db);

    return ret;
}

extern void
Grey_load_domains(Greylister_T greylister)
{
    char *domains_file = NULL;
    FILE *domains = NULL;
    char domain[GREY_MAX_MAIL], *dp, *sp, *addr;
    int overflow = 0;

    if(!((domains_file = Config_get_str(
            greylister->config, "permitted_domains", "grey", NULL)) != NULL
        && (domains = fopen(domains_file, "r")) != NULL))
    {
        return;
    }
    else if(domains_file && domains == NULL) {
        i_warning("failed to load domains from %s", domains_file);
        return;
    }

    if(greylister->domains)
        List_remove_all(greylister->domains);

    while(fgets(domain, sizeof(domain), domains) != NULL) {
        sp = domain;
        if((dp = strchr(domain, '\n')) == NULL) {
            overflow = 1;
            continue;
        }
        else if(overflow) {
            /* This is the tail end of an overly large line. */
            overflow = 0;
            continue;
        }

        /* Remove trailing white space. */
        do {
            dp--;
        }
        while(dp > sp && isspace(*dp));

        if(dp == sp)
            continue;
        else
            *(++dp) = '\0';

        /* Remove leading white space and skip comments. */
        while(sp != dp && isspace(*sp))
            sp++ ;

        if(*sp == '\0' || *sp == '#')
            continue;

        if((addr = strdup(sp)) == NULL)
            i_warning("error inserting domain: %s", strerror(errno));
        List_insert_after(greylister->domains, addr);
    }

    if(ferror(domains)) {
        i_warning("error while loading domains: %s",
                  strerror(errno));
    }
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
trap_check(Greylister_T greylister, struct Grey_tuple *gt)
{
    struct DB_key key;
    struct DB_val val;
    struct List_entry *entry;
    char *domain, *to = gt->to;
    int ret, to_len, from_pos, match = 0, check_domains = 0;

    /* If any plugin traps match, return straight away. */
    if(!Plugin_run_spamtraps(gt))
        return 0;

    if(List_size(greylister->domains) > 0) {
        check_domains = 1;
        to_len = strlen(to);
        LIST_EACH(greylister->domains, entry) {
            domain = List_entry_value(entry);
            from_pos = to_len - strlen(domain);

            if((from_pos >= 0)
               && (strcasecmp(to + from_pos, domain) == 0))
            {
                match = 1;
            }
        }
    }

    key.type = DB_KEY_DOM_PART;
    key.data.s = to;

    if(!match && Config_get_int(greylister->config, "db_permitted_domains",
                                "grey", DB_PERMITTED_DOM))
    {
        check_domains = 1;
        ret = DB_get(greylister->db_handle, &key, &val);
        if(ret == GREYDB_FOUND)
            match = 1;
    }

    if(check_domains && !match) {
        /* Trap this address. */
        return 0;
    }

    /*
     * Finally check if we have a direct hit on a pre-configured spamtrap
     * email address.
     */
    key.type = DB_KEY_MAIL;
    ret = DB_get(greylister->db_handle, &key, &val);
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
update_firewall(Greylister_T greylister, int af)
{
    List_T ips;
    struct List_entry *entry;
    char *ip, *name;
    int first = 1;

    if(af == AF_INET) {
        name = greylister->whitelist_name;
        ips = greylister->whitelist;
    }
    else {
        name = greylister->whitelist_name_ipv6;
        ips = greylister->whitelist_ipv6;
    }

    if(greylister->fw_out != NULL && List_size(ips) > 0) {
        fprintf(greylister->fw_out,
                "type=\"replace\"\n"
                "name=\"%s\"\n"
                "af=%u\n"
                "ips=[",
                name, af);

        LIST_EACH(ips, entry) {
            ip = List_entry_value(entry);
            fprintf(greylister->fw_out, "%s\"%s\"", (first ? "" : ","), ip);
            first = 0;
        }
        fprintf(greylister->fw_out, "]\n%%\n");

        if(fflush(greylister->fw_out) == EOF)
            i_debug("update firewall: fflush failed");
    }
}

static void
process_grey(Greylister_T greylister, struct Grey_tuple *gt, int sync,
             char *dst_ip)
{
    DB_handle_T db = greylister->db_handle;
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;
    time_t now, expire;
    int spamtrap;

    now = time(NULL);
    DB_open(db, 0);

    switch(trap_check(greylister, gt)) {
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

        /*
         * Making the pass time and expire time the same is significant, as
         * it means that a grey entry will not be whitelisted unless they
         * retry with the same "tuple".
         */
        gd.pass = now + expire;
        gd.expire = now + expire;
        gd.first = now;
        gd.bcount = 1;
        gd.pcount = (spamtrap ? -1 : 0);
        val.type = DB_VAL_GREY;
        val.data.gd = gd;

        switch(DB_put(db, &key, &val)) {
        case GREYDB_OK:
            if(sync) {
                i_debug("new %sentry %s from %s to %s, helo %s",
                        (spamtrap ? "greytrap " : ""), gt->ip,
                        gt->from, gt->to, gt->helo );
            }
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
            if(sync) {
                i_debug("updated %sentry %s from %s to %s, helo %s",
                        (spamtrap ? "greytrap " : ""), gt->ip,
                        gt->from, gt->to, gt->helo );
            }
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
    if(greylister->syncer && sync) {
        if(spamtrap) {
            Sync_trapped(greylister->syncer, gt->ip, now, now + expire, 0);
        }
        else {
            Sync_update(greylister->syncer, gt, now);
        }
    }

    DB_commit_txn(db);
    return;

rollback:
    DB_rollback_txn(db);
}

static void
process_non_grey(Greylister_T greylister, int spamtrap, char *ip, char *source,
                 char *expires, int sync, int delete)
{
    DB_handle_T db = greylister->db_handle;
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
    if(!delete && (expire == 0 || expires[0] == '\0' || *end != '\0'
           || (errno == ERANGE && (expire == LONG_MAX || expire == LONG_MIN))))
    {
        i_warning("could not parse expires %s", expires);
        return;
    }

    key.type = DB_KEY_IP;
    key.data.s = ip;

    DB_open(db, 0);
    DB_start_txn(db);

    switch(DB_get(db, &key, &val)) {
    case GREYDB_NOT_FOUND:
        /*
         * This is a new entry.
         */
        if(delete)
            break;

        memset(&gd, 0, sizeof(gd));
        gd.first = now;
        gd.pcount = (spamtrap ? -1 : 0);
        gd.pass = (spamtrap ? expire : now);
        gd.expire = expire;
        val.type = DB_VAL_GREY;
        val.data.gd = gd;

        if(DB_put(db, &key, &val) == GREYDB_OK && sync) {
            i_debug("new %s from %s for %s, expires %s",
                    (spamtrap ? "TRAP" : "WHITE"), source, ip,
                    expires);
        }
        break;

    case GREYDB_FOUND:
        /*
         * This is an existing entry.
         */
        if(delete) {
            if(DB_del(db, &key) == GREYDB_OK && sync) {
                i_debug("deleted %s", ip);
            }
        }
        else {
            if(spamtrap) {
                val.data.gd.pcount = -1;
                val.data.gd.bcount++;
            }
            else {
                val.data.gd.pcount++;
            }

            if(DB_put(db, &key, &val) == GREYDB_OK && sync) {
                i_debug("updated %s", ip);
            }
        }
        break;

    default:
        goto rollback;
        break;
    }

    DB_commit_txn(db);
    return;

rollback:
    DB_rollback_txn(db);
}

static int
process_message(Greylister_T greylister, Config_T message)
{
    int type, sync, delete;
    struct Grey_tuple gt;
    char *dst_ip, *ip, *source, *expires;

    type   = Config_get_int(message, "type", NULL, -1);
    dst_ip = Config_get_str(message, "dst_ip", NULL, "");

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
        if(gt.ip && gt.helo && gt.from && gt.to) {
            process_grey(greylister, &gt, sync, dst_ip);
        }
        break;

    case GREY_MSG_TRAP:
    case GREY_MSG_WHITE:
        ip      = Config_get_str(message, "ip", NULL, NULL);
        source  = Config_get_str(message, "source", NULL, NULL);
        expires = Config_get_str(message, "expires", NULL, NULL);
        delete  = Config_get_int(message, "delete", NULL, 0);
        if(ip && source && expires)
            process_non_grey(greylister, (type == GREY_MSG_TRAP ? 1 : 0),
                             ip, source, expires, sync, delete);
        break;

    default:
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
drop_grey_privs(Greylister_T greylister, struct passwd *user)
{
    if(user && Config_get_int(greylister->config, "drop_privs", NULL, 1)
       && drop_privs(user) == -1)
    {
        i_critical("failed to drop privileges");
    }
}

static void
shutdown_greyd(int sig)
{
    if(Grey_greylister) {
        Grey_greylister->shutdown = 1;
    }
}
