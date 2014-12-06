/**
 * @file   greyd.h
 * @brief  Defines greyd support constants and definitions.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef GREYD_DEFINED
#define GREYD_DEFINED

#include "hash.h"

#include <stdio.h>

/**
 * Structure to encapsulate the state of the main
 * greyd process.
 */
struct Greyd_state {
    Config_T config;
    int slow_until;

    struct Con *cons;
    int max_files;
    int max_cons;
    int max_black;
    int clients;
    int black_clients;

    FILE *grey_out;

    Hash_T blacklists;
};

/**
 * Process configuration input, and add resulting blacklist to the
 * state's list.
 */
extern void Greyd_process_config(int fd, struct Greyd_state *state);

/**
 * Send blacklist configuration to the specified file descriptor.
 */
extern void Greyd_send_config(FILE *out, char *bl_name, char *bl_msg, List_T ips);

#endif
