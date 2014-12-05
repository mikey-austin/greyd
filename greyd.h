/**
 * @file   greyd.h
 * @brief  Defines greyd support constants and definitions.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef GREYD_DEFINED
#define GREYD_DEFINED

#include "hash.h"

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

#endif
