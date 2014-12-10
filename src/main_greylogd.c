/**
 * @file   main_greylogd.c
 * @brief  Main function for the greylogd daemon.
 * @author Mikey Austin
 * @date   2014
 */

#define _GNU_SOURCE

#include "failures.h"
#include "config.h"
#include "constants.h"
#include "firewall.h"
#include "greydb.h"
#include "log.h"

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>

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
    char *config_file = NULL, *db_user, *ip;
    struct passwd *db_pw;
    int option, white_time;
    FW_handle_T fw_handle;
    DB_handle_T db_handle;

    tzset();
    opts = Config_create();

    while((option = getopt(argc, argv, "DIW:Y:")) != -1) {
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
            Config_set_int(opts, "inbound", "greylogd", 1);
            break;

        case 'W':
            /* Convert hours to seconds. */
            white_time = atoi(optarg);
            Config_set_int(opts, "white_expiry", "grey", (white_time * 60 * 60));
            break;

        case 'Y':
            /* TODO */
            break;

        default:
            usage();
            /* NOTREACHED */
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

    Log_setup(config, PROG_NAME);

    i_debug("Listening, %s",
            Config_get_int(config, "inbound", "greylogd", TRACK_INBOUND)
            ? "in both directions"
            : "inbound direction only");

    if((fw_handle = FW_open(config)) == NULL)
        i_critical("could not obtain firewall handle");

    if((db_handle = DB_open(config, 0)) == NULL)
        i_critical("could not obtain database handle");

    signal(SIGINT , sighandler_shutdown);
    signal(SIGQUIT, sighandler_shutdown);
    signal(SIGTERM, sighandler_shutdown);

    db_user = Config_get_str(config, "user", "grey", GREYD_DB_USER);
    if((db_pw = getpwnam(db_user)) == NULL)
        i_critical("no such user %s", db_user);

    if(!Config_get_int(config, "debug", NULL, 0)) {
        if(daemon(1, 1) == -1)
            err(1, "daemon");
    }

    if(db_pw && Config_get_int(config, "drop_privs", NULL, 1)) {
        if(setgroups(1, &db_pw->pw_gid)
           || setresgid(db_pw->pw_gid, db_pw->pw_gid, db_pw->pw_gid)
           || setresuid(db_pw->pw_uid, db_pw->pw_uid, db_pw->pw_uid))
        {
            i_critical("failed to drop privileges");
        }
    }

    FW_init_log_capture(fw_handle);

    for(;;) {
        if(Greylogd_shutdown)
            break;

        ip = FW_capture_log(fw_handle);
        free(ip);
    }

    i_info("exiting");
    FW_end_log_capture(fw_handle);
    FW_close(&fw_handle);
    DB_close(&db_handle);
    Config_destroy(&config);

    return 0;
}
