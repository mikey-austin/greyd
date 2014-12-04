/**
 * @file   main_greyd.c
 * @brief  Main function for the greyd daemon.
 * @author Mikey Austin
 * @date   2014
 */

#define _GNU_SOURCE

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
#include <signal.h>
#include <sys/resource.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>

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
    int max_files = CON_DEFAULT_MAX;

#ifdef __linux
    FILE *file_max = fopen("/proc/sys/fs/file-max", "r");
    if(file_max == NULL)
        err(1, "fopen");

    if(fscanf(file_max, "%d", &max_files) == EOF)
        err(1, "fscanf");
#endif

    return max_files;
}

int
main(int argc, char **argv)
{
    struct Greyd_state state;
    Greylister_T greylister;
    Config_T config, opts;
    char *config_file = NULL, hostname[MAX_HOST_NAME];
    char *bind_addr, *bind_addr6;
    int option, i, main_sock, main_sock6 = -1, cfg_sock, sock_val = 1;
    int grey_pipe[2], trap_pipe[2];
    u_short port, cfg_port, sync_port;
    long long grey_time, white_time, pass_time;
    struct rlimit limit;
    struct sockaddr_in main_addr, cfg_addr;
    struct sockaddr_in6 main_addr6;
    struct passwd *main_pw;
    char *main_user;
    pid_t grey_pid;
    FILE *grey_in, *trap_out;
    char *chroot_dir;

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
           getopt(argc, argv, "456f:l:L:c:B:p:bdG:h:s:S:M:n:vw:y:Y:")) != -1)
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

        case '6':
            Config_set_int(opts, "enable_ipv6", NULL, 1);
            break;

        case 'l':
            Config_set_str(opts, "bind_address", NULL, optarg);
            break;

        case 'L':
            Config_set_str(opts, "bind_address_ipv6", NULL, optarg);
            break;

        case 'B':
            i = atoi(optarg);
            if (i > state.max_files) {
                warnx("%d > system max of %d connections",
                      i, state.max_files);
                usage();
            }
            state.max_black = i;
            break;

        case 'c':
            i = atoi(optarg);
            if (i > state.max_files) {
                warnx("%d > system max of %d connections",
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

    if(!Config_get_int(state.config, "enable", "grey", GREYLISTING_ENABLED))
        state.max_black = state.max_cons;
    else if(state.max_black > state.max_cons)
        usage();

    limit.rlim_cur = limit.rlim_max = state.max_cons + 15;
    if(setrlimit(RLIMIT_NOFILE, &limit) == -1)
        err(1, "setrlimit");

    state.cons = calloc(state.max_cons, sizeof(*state.cons));
    if(state.cons == NULL)
        err(1, "calloc");

    signal(SIGPIPE, SIG_IGN);

    port = Config_get_int(state.config, "port", NULL, GREYD_PORT);
    cfg_port = Config_get_int(state.config, "config_port", NULL, GREYD_CFG_PORT);
    sync_port = Config_get_int(state.config, "port", "sync", GREYD_SYNC_PORT);

    /*
     * Setup the main IPv4 socket.
     */
    if((main_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err(1, "socket");

    if(setsockopt(main_sock, SOL_SOCKET, SO_REUSEADDR, &sock_val,
                  sizeof(sock_val)) == -1)
    {
        err(1, "setsockopt");
    }

    bind_addr = Config_get_str(state.config, "bind_address", NULL, NULL);
    memset(&main_addr, 0, sizeof(main_addr));
    if(bind_addr != NULL) {
        if(inet_pton(AF_INET, bind_addr, &main_addr.sin_addr) != 1)
            err(1, "inet_pton");
    }
    else {
        main_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    main_addr.sin_family = AF_INET;
    main_addr.sin_port = htons(port);

    if(bind(main_sock, (struct sockaddr *) &main_addr, sizeof(main_addr)) == -1)
        err(1, "bind");

    /*
     * Setup the main IPv6 socket if explicitly enabled.
     */
    if(Config_get_int(state.config, "enable_ipv6", NULL, IPV6_ENABLED)) {
        if((main_sock6 = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
            err(1, "socket");

        if(setsockopt(main_sock6, SOL_SOCKET, SO_REUSEADDR, &sock_val,
                      sizeof(sock_val)) == -1)
        {
            err(1, "setsockopt");
        }

        bind_addr6 = Config_get_str(state.config, "bind_address_ipv6", NULL, NULL);
        memset(&main_addr6, 0, sizeof(main_addr6));
        if(bind_addr6 != NULL) {
            if(inet_pton(AF_INET6, bind_addr6, &main_addr6.sin6_addr) != 1)
                err(1, "inet_pton");
        }
        else {
            main_addr6.sin6_addr = in6addr_any;
        }
        main_addr6.sin6_family = AF_INET6;
        main_addr6.sin6_port = htons(port);

        if(bind(main_sock6, (struct sockaddr *) &main_addr6, sizeof(main_addr6)) == -1)
            err(1, "bind IPv6");
    }

    /*
     * Setup the configuration socket to only listen for connections on loopback.
     */
    if((cfg_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err(1, "socket");

    if(setsockopt(cfg_sock, SOL_SOCKET, SO_REUSEADDR, &sock_val,
                  sizeof(sock_val)) == -1)
    {
        err(1, "setsockopt");
    }

    memset(&cfg_addr, 0, sizeof(cfg_addr));
    cfg_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    cfg_addr.sin_family = AF_INET;
    cfg_addr.sin_port = htons(cfg_port);

    if(bind(cfg_sock, (struct sockaddr *) &cfg_addr, sizeof(cfg_addr)) == -1)
        err(1, "bind local");

    main_user = Config_get_str(state.config, "user", NULL, GREYD_MAIN_USER);
    if((main_pw = getpwnam(main_user)) == NULL)
        errx(1, "no such user %s", main_user);

    if(!Config_get_int(state.config, "debug", NULL, 0)) {
        if(daemon(1, 1) == -1)
            err(1, "daemon");
    }

    if(Config_get_int(state.config, "enable", "grey", 0)) {
        /* Ensure that the the grey connections outweigh the blacklisted. */
        state.max_black = (state.max_black >= state.max_cons
                           ? state.max_cons - 100 : state.max_black);
        if(state.max_black < 0) {
            I_WARN("maximum blacklisted connections is 0");
            state.max_black = 0;
        }

        if(pipe(grey_pipe) == -1)
            I_CRIT("grey pipe: %m");

        if(pipe(trap_pipe) == -1)
            I_CRIT("trap pipe: %m");

        grey_pid = fork();
        switch(grey_pid) {
        case -1:
            I_ERR("fork greylister: %m");

        case 0:
            /* In child. */
            signal(SIGPIPE, SIG_IGN);

            if((state.grey_out = fdopen(grey_pipe[1], "w")) == NULL)
                I_CRIT("fdopen: %m");
            close(grey_pipe[0]);

            if((state.trap_in = fdopen(trap_pipe[0], "r")) == NULL)
                I_CRIT("fdopen: %m");
            close(trap_pipe[1]);

            goto jail;
        }

        /* In parent. Run the greylisting engine. */
        greylister = Grey_setup(state.config);

        if((grey_in = fdopen(grey_pipe[0], "r")) == NULL)
            I_CRIT("fdopen: %m");
        close(grey_pipe[1]);

        if((trap_out = fdopen(trap_pipe[1], "w")) == NULL)
            I_CRIT("fdopen: %m");
        close(trap_pipe[0]);

        Grey_start(greylister, grey_pid, grey_in, trap_out);
        /* Not reached. */
    }

jail:
    if(Config_get_int(state.config, "chroot", NULL, GREYD_CHROOT)) {
        chroot_dir = Config_get_str(state.config, "chroot_dir", NULL,
                                    GREYD_CHROOT_DIR);
        if(chroot(chroot_dir) == -1)
            I_CRIT("cannot chroot to %s", chroot_dir);
    }

    if(main_pw) {
        if(setgroups(1, &main_pw->pw_gid)
           || setresgid(main_pw->pw_gid, main_pw->pw_gid, main_pw->pw_gid)
           || setresuid(main_pw->pw_uid, main_pw->pw_uid, main_pw->pw_uid))
        {
            I_CRIT("failed to drop privileges");
        }
    }

    if(listen(main_sock, GREYD_BACKLOG) == -1)
        I_CRIT("listen: %m");

    if(listen(cfg_sock, GREYD_BACKLOG) == -1)
        I_CRIT("listen: %m");

    if(main_sock6 > 0) {
        if(listen(main_sock6, GREYD_BACKLOG) == -1)
            I_CRIT("listen: %m");
    }

    I_WARN("listening for incoming connections");

    for(;;) {
    }

    /* Not reached. */
}
