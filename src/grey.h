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
 * @file   grey.h
 * @brief  Defines the greylisting management interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef GREY_DEFINED
#define GREY_DEFINED

#include <config.h>

#include <sys/types.h>

#ifdef HAVE_SPF
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <net/if.h>
#endif
#ifdef HAVE_SPF2_SPF_H
#  include <spf2/spf.h>
#endif
#ifdef HAVE_SPF_H
#  include <spf.h>
#endif

#include <stdio.h>

#include "firewall.h"
#include "greyd_config.h"
#include "list.h"

#define GREY_MAX_MAIL         1024
#define GREY_MAX_KEY          45
#define GREY_DB_SCAN_INTERVAL 60
#define GREY_DB_TRAP_INTERVAL (60 * 10)

#define GREY_MSG_GREY  1
#define GREY_MSG_TRAP  2
#define GREY_MSG_WHITE 3

/**< Pass after first retry seen after 25 minutes. */
#define GREY_PASSTIME (60 * 25)

/**< Remove grey entries after 4 hours. */
#define GREY_GREYEXP  (60 * 60 * 4)

/**< Remove white entries after 36 days. */
#define GREY_WHITEEXP (60 * 60 * 24 * 36)

/**< Hitting a spamtrap blacklists for a day. */
#define GREY_TRAPEXP  (60 * 60 * 24)

struct Grey_tuple {
    char *ip;
    char *helo;
    char *from;
    char *to;
};

struct Grey_data {
	int64_t first;  /**< When did we see it first. */
	int64_t pass;   /**< When was it whitelisted. */
	int64_t expire; /**< When will we get rid of this entry. */
	int     bcount; /**< How many times have we blocked it. */
	int     pcount; /**< How many times passed, or -1 for spamtrap. */
};

struct DB_handle_T;
struct Sync_engine_T;

/**
 * The greylister holds the state of the greylisting engine.
 */
typedef struct Greylister_T *Greylister_T;
struct Greylister_T {
    Config_T  config;
    int       shutdown;
    char     *low_prio_mx;
    char     *traplist_name;
    char     *traplist_msg;
    char     *whitelist_name;
    char     *whitelist_name_ipv6;
    List_T    whitelist;
    List_T    whitelist_ipv6;
    List_T    traplist;
    List_T    domains;
    FILE     *trap_out;
    FILE     *grey_in;
    FILE     *fw_out;
    pid_t     grey_pid;
    pid_t     reader_pid;
    pid_t     fw_pid;
    time_t    startup;
    time_t    grey_exp;
    time_t    trap_exp;
    time_t    white_exp;
    time_t    pass_time;

    struct DB_handle_T   *db_handle;
    struct Sync_engine_T *syncer;

#ifdef HAVE_SPF
    SPF_server_t *spf_server;
#endif
};

/**
 * Given a configuration structure, setup and initialize and
 * return the greylisting engine state.
 */
extern Greylister_T Grey_setup(Config_T config);

/**
 * Start the greylisting engine.
 */
extern void Grey_start(Greylister_T greylister, pid_t grey_pid,
                       FILE *grey_in, FILE *trap_out, FILE *fw_out);

/**
 * Stop the greylisting engine and cleanup afterwards.
 */
extern void Grey_finish(Greylister_T *greylister);

/**
 * Start the child process which reads data from the main
 * greyd server process, and updates the database accordingly.
 */
extern int Grey_start_reader(Greylister_T greylister);

/**
 * Start the child process which periodically scans the greyd
 * database to update counters, expire entries, configure
 * whitelists and traplists.
 */
extern void Grey_start_scanner(Greylister_T greylister);

/**
 * Scan the grey engine database looking to expire entries,
 * update firewall whitelists and send trapped IP addresses to
 * the main greyd process.
 */
extern int Grey_scan_db(Greylister_T greylister);

/**
 * Loads the permitted domains from the configured file path in
 * the configuration.
 */
extern void Grey_load_domains(Greylister_T greylister);

#endif
