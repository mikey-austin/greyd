/**
 * @file   main_greyd.c
 * @brief  Main function for the greyd daemon.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "config.h"
#include "constants.h"
#include "grey.h"
#include "ip.h"
#include "utils.h"
#include "con.h"
#include "greyd.h"

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

static void usage(void);
static int max_files(void);

static void
usage(void)
{
    fprintf(stderr,
        "usage: %s [-f config] [-45bdv] [-B maxblack] [-c maxcon] "
        "[-G passtime:greyexp:whiteexp]\n"
        "\t[-h hostname] [-l address] [-M address] [-n name] [-p port]\n"
        "\t[-S secs] [-s secs] "
        "[-w window] [-Y synctarget] [-y synclisten]\n",
        PROG_NAME);

    exit(1);
}

static int
max_files(void)
{
    return 1000; // TODO
}

int
main(int argc, char **argv)
{
    struct Greyd_state state;
    Config_T config, opts;
    char *config_file = NULL, hostname[MAX_HOST_NAME];
    int option, i;
    long long grey_time, white_time, pass_time;

    tzset();

    opts = Config_create();
    if(gethostname(hostname, sizeof(hostname)) == -1) {
        err(1, "gethostname");
    }
    else {
        /* Set the default to the output of gethostname. */
        Config_set_str(opts, "hostname", NULL, hostname);
    }

    state.max_files = max_files();
    state.max_cons = (CON_DEFAULT_MAX > state.max_files
                      ? state.max_files : CON_DEFAULT_MAX);
    state.max_black = (CON_DEFAULT_MAX > state.max_files
                      ? state.max_files : CON_DEFAULT_MAX);

    while((option =
           getopt(argc, argv, "45l:c:B:p:bdG:h:s:S:M:n:vw:y:Y:")) != -1)
    {
        switch(option) {
        case 'f':
            config_file = optarg;
            break;

        case '4':
            Config_set_str(opts, "error_code", NULL, "450");
            break;

        case '5':
            Config_set_str(opts, "error_code", NULL, "550");
            break;

        case 'l':
            Config_set_str(opts, "bind_address", NULL, optarg);
            break;

        case 'B':
            i = atoi(optarg);
            if (i > state.max_files) {
                warnx("%d > system max of %lu connections",
                      i, state.max_files);
                usage();
            }
            state.max_black = i;
            break;

        case 'c':
            i = atoi(optarg);
            if (i > state.max_files) {
                warnx("%d > system max of %lu connections",
                      i, state.max_files);
                usage();
            }
            state.max_cons = i;
            break;

        case 'p':
            Config_set_int(opts, "port", NULL, atoi(optarg));
            break;

        case 'd':
            Config_set_int(opts, "debug", NULL, 1);
            break;

        case 'b':
            Config_set_int(opts, "enable", "grey", 0);
            break;

        case 'G':
            if (sscanf(optarg, "%lld:%lld:%lld", &pass_time, &grey_time,
                       &white_time) != 3)
            {
                usage();
            }

            /* Convert minutes to seconds. */
            Config_set_int(opts, "pass_time", "grey", (pass_time * 60));

            /* Convert hours to seconds. */
            Config_set_int(opts, "grey_expiry", "grey", (grey_time * 60 * 60));
            Config_set_int(opts, "white_expiry", "grey", (white_time * 60 * 60));
            break;

        case 'h':
            memset(hostname, 0, sizeof(hostname));
            if(sstrncpy(hostname, optarg, sizeof(hostname)) >=
               sizeof(hostname))
            {
                warnx("-h arg too long");
                usage();
            }
            Config_set_str(opts, "hostname", NULL, hostname);
            break;

        case 's':
            i = atoi(optarg);
            if(i < 0 || i > (10 * CON_STUTTER))
                usage();
            Config_set_int(opts, "stutter", NULL, i);
            break;

        case 'S':
            i = atoi(optarg);
            if(i < 0 || i > (10 * CON_GREY_STUTTER))
                usage();
            Config_set_int(opts, "stutter", "grey", i);
            break;

        case 'M':
            Config_set_str(opts, "low_prio_mx_ip", NULL, optarg);
            break;

        case 'n':
            Config_set_str(opts, "banner", NULL, optarg);
            break;

        case 'v':
            Config_set_int(opts, "verbose", NULL, 1);
            break;

        case 'w':
            i = atoi(optarg);
            if(i <= 0)
                usage();
            Config_set_int(opts, "window", NULL, i);
            break;

        case 'Y':
            // TODO
            break;

        case 'y':
            // TODO
            break;

        default:
            usage();
            break;
        }
    }

    if(config_file != NULL) {
        config = Config_create();
        Config_load_file(config, config_file);
        Config_merge(config, opts);
        Config_destroy(&opts);
        state.config = config;
    }
    else {
        state.config = opts;
    }
}
