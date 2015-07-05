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
 * @file   main_greyd.c
 * @brief  Main function for the greyd daemon.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#include <sys/types.h>
#include <sys/resource.h>

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
#ifdef HAVE_LIBEV_EV_H
#  include <libev/ev.h>
#endif
#ifdef HAVE_EV_H
#  include <ev.h>
#endif

#include "failures.h"
#include "greyd_config.h"
#include "constants.h"
#include "grey.h"
#include "sync.h"
#include "ip.h"
#include "utils.h"
#include "con.h"
#include "greyd.h"
#include "hash.h"
#include "log.h"
#include "config_parser.h"
#include "lexer_source.h"
#include "events.h"

#define PROG_NAME "greyd"

extern char *optarg;
extern int optind, opterr, optopt;

static void usage(void);
static int max_files(void);
static void destroy_blacklist(struct Hash_entry *entry);
static void shutdown_greyd(int sig);

struct Greyd_state *Greyd_state = NULL;

static void
usage(void)
{
    fprintf(stderr,
        "usage: %s [-f config] [-45bdv] [-B maxblack] [-c maxcon] "
        "[-G passtime:greyexp:whiteexp]\n"
        "\t[-h hostname] [-l address] [-M address] [-n name] [-p port]\n"
        "\t[-P pidfile] [-S secs] [-s secs] [-L ipv6 address]\n"
        "[-w window] [-Y synctarget] [-y synclisten]\n",
        PROG_NAME);

    exit(1);
}

static void
shutdown_greyd(int sig)
{
    if(Greyd_state)
        Greyd_state->shutdown = 1;
}

static int
max_files(void)
{
    int max_files = CON_DEFAULT_MAX;

#ifdef __linux
    FILE *file_max = fopen("/proc/sys/fs/file-max", "r");
    if(file_max == NULL || fscanf(file_max, "%d", &max_files) == EOF) {
        if(file_max)
            fclose(file_max);
        return max_files;
    }
    fclose(file_max);
#endif

    if((max_files - MAX_FILES_THRESHOLD) < 10)
        errx(1, "max files is only %d, refusing to continue", max_files);
    else
        return (max_files - MAX_FILES_THRESHOLD);
}

static void
destroy_blacklist(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        Blacklist_destroy((Blacklist_T *) &entry->v);
    }
}

int
main(int argc, char **argv)
{
    struct Greyd_state state;
    Greylister_T greylister;
    Config_T config, opts;
    char *config_file = DEFAULT_CONFIG, hostname[MAX_HOST_NAME];
    char *bind_addr, *bind_addr6, *pidfile;
    int option, i, main_sock, main_sock6 = -1, cfg_sock, sock_val = 1;
    int grey_pipe[2], trap_pipe[2], trap_fd = -1, cfg_fd = -1;
    int fw_pipe[2], nat_pipe[2], grey_fw_pipe[2];
    u_short port, cfg_port;
    unsigned long long grey_time, white_time, pass_time;
    struct rlimit limit;
    struct sockaddr_in main_addr, cfg_addr, main_in_addr;
    struct sockaddr_in6 main_addr6, main_in_addr6;
    struct passwd *main_pw;
    List_T hosts;
    char *main_user;
    pid_t grey_pid;
    FILE *grey_in, *trap_out, *grey_fw;
    char *chroot_dir = NULL;
    int prev_max_fd = 0, sync_recv = 0, sync_send = 0;
    struct sigaction sa;

    opts = Config_create();
    if(gethostname(hostname, sizeof(hostname)) == -1) {
        err(1, "gethostname");
    }
    else {
        /* Set the default to the output of gethostname. */
        Config_set_str(opts, "hostname", NULL, hostname);
    }

    state.shutdown = 0;
    state.max_files = max_files();

    /* Global reference only to be used for signal handlers. */
    Greyd_state = &state;

    while((option = getopt(argc, argv,
                           "456f:l:L:c:B:p:bdG:h:s:S:M:n:vw:y:Y:P:")) != -1)
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
            Config_set_int(opts, "max_cons_black", NULL, i);
            break;

        case 'c':
            i = atoi(optarg);
            if (i > state.max_files) {
                warnx("%d > system max of %d connections",
                      i, state.max_files);
                usage();
            }
            Config_set_int(opts, "max_cons", NULL, i);
            break;

        case 'p':
            Config_set_int(opts, "port", NULL, atoi(optarg));
            break;

        case 'P':
            Config_set_str(opts, "greyd_pidfile", NULL, optarg);
            break;

        case 'd':
            Config_set_int(opts, "debug", NULL, 1);
            break;

        case 'b':
            Config_set_int(opts, "enable", "grey", 0);
            break;

        case 'G':
            if(sscanf(optarg, "%llu:%llu:%llu", &pass_time, &grey_time,
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
            Config_set_str(opts, "low_prio_mx", "grey", optarg);
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
            Config_append_list_str(opts, "hosts", "sync", optarg);
            sync_send++;
            break;

        case 'y':
            Config_set_str(opts, "bind_address", "sync", optarg);
            sync_recv++;
            break;

        default:
            usage();
            break;
        }
    }

    config = Config_create();
    Config_load_file(config, config_file);
    Config_merge(config, opts);
    Config_destroy(&opts);
    state.config = config;
    state.loop = EV_DEFAULT;

    i = Config_get_int(config, "max_cons", NULL, CON_DEFAULT_MAX);
    state.max_cons = (i > state.max_files ? state.max_files : i);
    i = Config_get_int(config, "max_cons_black", NULL, CON_DEFAULT_MAX);
    state.max_black = (i > state.max_files ? state.max_files : i);

    if(sync_send == 0
       && (hosts = Config_get_list(state.config, "hosts", "sync")))
    {
        sync_send += List_size(hosts);
    }

    if(sync_recv == 0
       && Config_get_str(state.config, "bind_address", "sync", NULL) != NULL)
    {
        sync_recv = 1;
    }

    Log_setup(state.config, PROG_NAME);

    if(!Config_get_int(state.config, "enable", "grey", GREYLISTING_ENABLED)) {
        state.max_black = state.max_cons;
    }
    else if(state.max_black > state.max_cons) {
        warnx("Max black cons (%u) must not exceed total max cons (%u)",
              state.max_black, state.max_cons);
        usage();
    }

    if(Config_get_int(state.config, "setrlimit", NULL, SETRLIMIT)) {
        limit.rlim_cur = limit.rlim_max = state.max_cons + 15;
        if(setrlimit(RLIMIT_NOFILE, &limit) == -1)
            err(1, "setrlimit");
    }

    signal(SIGPIPE, SIG_IGN);

    port = Config_get_int(state.config, "port", NULL, GREYD_PORT);
    cfg_port = Config_get_int(state.config, "config_port", NULL, GREYD_CFG_PORT);

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

    if(Config_get_int(state.config, "daemonize", NULL, 1)) {
        if(daemon(1, 0) == -1)
            err(1, "daemon");
    }

    state.fw_out = NULL;
    state.fw_in = NULL;
    state.fw_pid = -1;

    main_user = Config_get_str(state.config, "user", NULL, GREYD_MAIN_USER);
    if((main_pw = getpwnam(main_user)) == NULL)
        errx(1, "no such user %s", main_user);

    pidfile = Config_get_str(state.config, "greyd_pidfile", NULL,
                             GREYD_PIDFILE);
    switch(write_pidfile(main_pw, pidfile)) {
    case -1:
        i_critical("could not write pidfile %s: %s", pidfile,
                  strerror(errno));

    case -2:
        i_critical("it appears greyd is already running...", pidfile);
    }

    if(Config_get_int(state.config, "enable", "grey", GREYLISTING_ENABLED)) {
        if(pipe(fw_pipe) == -1)
            i_critical("firewall pipe: %s", strerror(errno));

        if(pipe(nat_pipe) == -1)
            i_critical("firewall nat pipe: %s", strerror(errno));

        if(pipe(grey_fw_pipe) == -1)
            i_critical("grey firewall pipe: %s", strerror(errno));

        /* Fork the firewall process. */
        state.fw_pid = fork();
        switch(state.fw_pid) {
        case -1:
            i_error("fork firewall failed: %s", strerror(errno));

        case 0:
            /* In child. */
            memset(&sa, 0, sizeof(sa));
            sigfillset(&sa.sa_mask);
            sa.sa_handler = shutdown_greyd;
            sigaction(SIGTERM, &sa, NULL);
            sigaction(SIGHUP, &sa, NULL);
            sigaction(SIGINT, &sa, NULL);

            close(nat_pipe[0]);
            close(fw_pipe[1]);
            close(grey_fw_pipe[1]);

            return Greyd_start_fw_child(
                &state, grey_fw_pipe[0], fw_pipe[0], nat_pipe[1]);
        }

        /* In parent. */
        if((state.fw_out = fdopen(fw_pipe[1], "w")) == NULL)
            i_critical("fdopen: %s", strerror(errno));
        close(fw_pipe[0]);

        if((state.fw_in = fdopen(nat_pipe[0], "r")) == NULL)
            i_critical("fdopen: %s", strerror(errno));
        close(nat_pipe[1]);

        /* Ensure that the the grey connections outweigh the blacklisted. */
        state.max_black = (state.max_black >= state.max_cons
                           ? state.max_cons - 100 : state.max_black);
        if(state.max_black < 0) {
            i_warning("maximum blacklisted connections is 0");
            state.max_black = 0;
        }

        if(pipe(grey_pipe) == -1)
            i_critical("grey pipe: %s", strerror(errno));

        if(pipe(trap_pipe) == -1)
            i_critical("trap pipe: %s", strerror(errno));

        grey_pid = fork();
        switch(grey_pid) {
        case -1:
            i_error("fork greylister: %s", strerror(errno));

        case 0:
            /* In child. */
            signal(SIGPIPE, SIG_IGN);

            if((state.grey_out = fdopen(grey_pipe[1], "w")) == NULL)
                i_critical("fdopen: %s", strerror(errno));
            close(grey_pipe[0]);

            trap_fd = trap_pipe[0];
            close(trap_pipe[1]);
            close(grey_fw_pipe[1]);

            goto jail;
        }

        greylister = Grey_setup(state.config);

        if((grey_in = fdopen(grey_pipe[0], "r")) == NULL)
            i_critical("fdopen: %s", strerror(errno));
        close(grey_pipe[1]);

        if((trap_out = fdopen(trap_pipe[1], "w")) == NULL)
            i_critical("fdopen: %s", strerror(errno));
        close(trap_pipe[0]);

        if((grey_fw = fdopen(grey_fw_pipe[1], "w")) == NULL)
            i_critical("fdopen: %s", strerror(errno));
        close(grey_fw_pipe[0]);

        Grey_start(greylister, grey_pid, grey_in, trap_out, grey_fw);

        /* Not reached. */
    }

jail:
    /*
     * We only process sync messages in this process. As we don't
     * send them, delete the sync hosts config.
     */
    Config_delete(state.config, "hosts", "sync");

    if((sync_send || sync_recv)
       && (state.syncer = Sync_init(state.config)) == NULL)
    {
        i_warning("sync disabled by configuration");
        sync_send = 0;
        sync_recv = 0;
    }
    else if(state.syncer && Sync_start(state.syncer) == -1) {
        i_warning("could not start sync engine");
        Sync_stop(&state.syncer);
        sync_send = 0;
        sync_recv = 0;
    }

    ev_io sync_watcher;
    if(state.syncer) {
        sync_watcher.data = &state;
        ev_io_init(&sync_watcher, Events_sync_cb, state.syncer->sync_fd, EV_READ);
        ev_io_start(state.loop, &sync_watcher);
    }

    ev_signal hup_watcher, int_watcher, term_watcher;
    hup_watcher.data = int_watcher.data = term_watcher.data = &state;
    ev_signal_init(&hup_watcher, Events_signal_cb, SIGHUP);
    ev_signal_init(&int_watcher, Events_signal_cb, SIGINT);
    ev_signal_init(&term_watcher, Events_signal_cb, SIGTERM);
    ev_signal_start(state.loop, &hup_watcher);
    ev_signal_start(state.loop, &int_watcher);
    ev_signal_start(state.loop, &term_watcher);

    if(Config_get_int(state.config, "chroot", NULL, GREYD_CHROOT)) {
        tzset();
        chroot_dir = Config_get_str(state.config, "chroot_dir", NULL,
                                    GREYD_CHROOT_DIR);
        if(chroot(chroot_dir) == -1)
            i_critical("cannot chroot to %s", chroot_dir);
    }

    if(main_pw && Config_get_int(state.config, "drop_privs", NULL, 1)
       && drop_privs(main_pw) == -1)
    {
        i_critical("failed to drop privileges: %s", strerror(errno));
    }

    if(listen(main_sock, GREYD_BACKLOG) == -1)
        i_critical("listen: %s", strerror(errno));
    ev_io ipv4_watcher;
    ipv4_watcher.data = &state;
    ev_io_init(&ipv4_watcher, Events_accept_cb, main_sock, EV_READ);
    ev_io_start(state.loop, &ipv4_watcher);

    state.cfg_watcher = NULL;
    if(listen(cfg_sock, GREYD_BACKLOG) == -1)
        i_critical("listen: %s", strerror(errno));
    ev_io cfg_sock_watcher;
    cfg_sock_watcher.data = &state;
    ev_io_init(&cfg_sock_watcher, Events_config_accept_cb, cfg_sock, EV_READ);
    ev_io_start(state.loop, &cfg_sock_watcher);

    i_warning("listening for incoming connections");

    ev_io ipv6_watcher;
    if(main_sock6 > 0) {
        ipv6_watcher.data = &state;
        if(listen(main_sock6, GREYD_BACKLOG) == -1)
            i_critical("listen: %s", strerror(errno));
        ev_io_init(&ipv6_watcher, Events_accept_cb, main_sock6, EV_READ);
        ev_io_start(state.loop, &ipv6_watcher);

        i_warning("listening for incoming IPv6 connections");
    }

    ev_io trap_watcher;
    if(trap_fd > 0) {
        trap_watcher.data = &state;
        ev_io_init(&trap_watcher, Events_trap_cb, trap_fd, EV_READ);
        ev_io_start(state.loop, &trap_watcher);
    }

    state.slow_for = 0;
    state.clients = state.black_clients = 0;
    state.blacklists = Hash_create(NUM_BLACKLISTS, destroy_blacklist);

    for(;;) {
        if(!ev_run(state.loop, 0)) {
            i_debug("unexpected ev_run break");
            break;
        }

        if(state.slow_for > 0) {
            ev_sleep(state.slow_for);
            state.slow_for = 0;
        }

        if(state.shutdown)
            break;
    }

shutdown:
    i_debug("stopping main process");

    if(state.syncer)
        Sync_stop(&state.syncer);

    if(state.fw_pid != -1)
        kill(state.fw_pid, SIGTERM);

    close_pidfile(pidfile, chroot_dir);
    fclose(state.grey_out);
    Hash_destroy(&state.blacklists);
    Config_destroy(&state.config);

    return 0;
}
