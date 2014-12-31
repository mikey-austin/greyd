/**
 * @file   main_greylogd.c
 * @brief  Main function for the greylogd daemon.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

#include "failures.h"
#include "greyd_config.h"
#include "constants.h"
#include "firewall.h"
#include "greydb.h"
#include "log.h"
#include "sync.h"

#define PROG_NAME "greylogd"

extern char *optarg;
extern int optind, opterr, optopt;

static int Greylogd_shutdown = 0;

void
usage(void)
{
    fprintf(stderr,
            "usage: %s [-dI] [-f config] [-W whiteexp] "
            "[-Y synctarget] [-p syncport]\n",
            PROG_NAME);
    exit(1);
}

void
sighandler_shutdown(int signal)
{
    /* Signal a shutdown. */
    Greylogd_shutdown = 1;
}

int
main(int argc, char **argv)
{
    Config_T config, opts;
    char *config_file = NULL, *db_user;
    struct passwd *db_pw;
    int option, white_expiry, sync_send = 0;
    FW_handle_T fw_handle;
    List_T entries, hosts;
    struct List_entry *entry;
    DB_handle_T db_handle;
    struct DB_key key;
    struct DB_val val;
    time_t now;
    Sync_engine_T syncer = NULL;

    tzset();
    opts = Config_create();

    while((option = getopt(argc, argv, "DIW:Y:f:")) != -1) {
        switch (option) {
        case 'f':
            config_file = optarg;
            break;

        case 'p':
            Config_set_int(opts, "port", "sync", atoi(optarg));
            break;

        case 'd':
            Config_set_int(opts, "debug", NULL, 1);
            break;

        case 'I':
            Config_set_int(opts, "track_outbound", "firewall", 0);
            break;

        case 'W':
            /* Convert hours to seconds. */
            white_expiry = atoi(optarg);
            Config_set_int(opts, "white_expiry", "grey",
                           (white_expiry * 60 * 60));
            break;

        case 'Y':
            Config_append_list_str(opts, "hosts", "sync", optarg);
            sync_send++;
            break;

        default:
            usage();
            /* Not Reached. */
        }
    }

    if(config_file != NULL) {
        config = Config_create();
        Config_load_file(config, config_file);
        Config_merge(config, opts);
        Config_destroy(&opts);
    }
    else {
        config = opts;
    }

    if(sync_send == 0
       && (hosts = Config_get_list(config, "hosts", "sync")))
    {
        sync_send += List_size(hosts);
    }

    if(sync_send && (syncer = Sync_init(config)) == NULL) {
        i_warning("sync disabled by configuration");
        sync_send = 0;
    }
    else if(syncer && Sync_start(syncer) == -1) {
        i_warning("could not start sync engine");
        Sync_stop(&syncer);
        sync_send = 0;
    }

    Log_setup(config, PROG_NAME);

    i_info("Listening, %s",
           Config_get_int(config, "track_outbound", "firewall", TRACK_OUTBOUND)
           ? "in both directions" : "inbound direction only");

    if((fw_handle = FW_open(config)) == NULL)
        i_critical("could not obtain firewall handle");

    if((db_handle = DB_init(config)) == NULL)
        i_critical("could not obtain database handle");

    signal(SIGINT , sighandler_shutdown);
    signal(SIGQUIT, sighandler_shutdown);
    signal(SIGTERM, sighandler_shutdown);

    db_user = Config_get_str(config, "user", "grey", GREYD_DB_USER);
    if((db_pw = getpwnam(db_user)) == NULL) {
        i_warning("getpwnam: %s", strerror(errno));
        goto shutdown;
    }

    if(Config_get_int(config, "daemonize", NULL, 1)) {
        if(daemon(1, 1) == -1) {
            i_warning("daemon: %s", strerror(errno));
            goto shutdown;
        }
    }

    if(db_pw && Config_get_int(config, "drop_privs", NULL, 1)) {
        if(setgroups(1, &db_pw->pw_gid)
           || setresgid(db_pw->pw_gid, db_pw->pw_gid, db_pw->pw_gid)
           || setresuid(db_pw->pw_uid, db_pw->pw_uid, db_pw->pw_uid))
        {
            i_warning("could not drop privileges: %s", strerror(errno));
            goto shutdown;
        }
    }

    white_expiry = Config_get_int(config, "white_expiry", "grey", GREY_WHITEEXP);
    DB_open(db_handle, 0);
    FW_start_log_capture(fw_handle);

    while(!Greylogd_shutdown) {
        if((entries = FW_capture_log(fw_handle)) != NULL
           && List_size(entries) > 0)
        {
            now = time(NULL);
            LIST_FOREACH(entries, entry) {
                key.data.s = List_entry_value(entry);
                key.type = DB_KEY_IP;

                DB_start_txn(db_handle);
                switch(DB_get(db_handle, &key, &val)) {
                case GREYDB_NOT_FOUND:
                    /* Create new entry. */
                    memset(&val.data.gd, 0, sizeof(val.data.gd));
                    val.type = DB_VAL_GREY;
                    val.data.gd.first = now;
                    val.data.gd.pass = now;
                    /* Fallthrough. */

                case GREYDB_FOUND:
                    /* Update existing entry. */
                    val.data.gd.pcount++;
                    val.data.gd.expire = now + white_expiry;
                    if(DB_put(db_handle, &key, &val) != GREYDB_OK) {
                        i_warning("error putting %s", key.data.s);
                        DB_rollback_txn(db_handle);
                        goto shutdown;
                    }
                    else {
                        i_info("whitelisting %s", key.data.s);
                        if(sync_send)
                            Sync_white(syncer, key.data.s, now, now + white_expiry);
                    }
                    break;

                default:
                    i_warning("error querying database for %s", key.data.s);
                    DB_rollback_txn(db_handle);
                    goto shutdown;
                }

                DB_commit_txn(db_handle);
            }
        }
    }

shutdown:
    i_info("exiting");
    FW_end_log_capture(fw_handle);
    FW_close(&fw_handle);
    DB_close(&db_handle);
    Config_destroy(&config);
    if(sync_send)
        Sync_stop(&syncer);

    return 0;
}
