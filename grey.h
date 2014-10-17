/**
 * @file   grey.h
 * @brief  Defines the greylisting management interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef GREY_DEFINED
#define GREY_DEFINED

#include "config.h"
#include "list.h"

#include <stdio.h>

#define T Grey_tuple
#define D Grey_data
#define G Greylister_T

#define GREY_MAX_MAIL 1024
#define GREY_MAX_KEY  45
#define GREY_PASSTIME (60 * 25) /* pass after first retry seen after 25 mins */
#define GREY_GREYEXP  (60 * 60 * 4) /* remove grey entries after 4 hours */
#define GREY_WHITEEXP (60 * 60 * 24 * 36) /* remove white entries after 36 days */
#define GREY_TRAPEXP  (60 * 60 * 24) /* hitting a spamtrap blacklists for a day */

struct T {
    char *ip;
    char *helo;
    char *from;
    char *to;
};

struct D {
	int64_t first;  /**< When did we see it first. */
	int64_t pass;   /**< When was it whitelisted. */
	int64_t expire; /**< When will we get rid of this entry. */
	int     bcount; /**< How many times have we blocked it. */
	int     pcount; /**< How many times passed, or -1 for spamtrap. */
};

/**
 * The greylister holds the state of the greylisting engine.
 */
struct G {
    Config_T  config;
    char     *traplist_name;
    char     *traplist_msg;
    List_T    whitelist;
    List_T    traplist;
    FILE     *trap_out;
    FILE     *grey_in;
    pid_t     jail_pid;
    pid_t     db_pid;
};

/**
 * Given a configuration structure, initialize and start the
 * grey listing engine.
 */
extern G Grey_start(Config_T config);

/**
 * Start the child process which reads data from the main
 * greyd server process, and updates the database accordingly.
 */
extern int Grey_start_reader(G greylister);

/**
 * Start the child process which periodically scans the greyd
 * database to update counters, expire entries, configure
 * whitelists and traplists.
 */
extern int Grey_start_scanner(G greylister);

#undef T
#undef D
#endif
