/**
 * @file   greyd.h
 * @brief  Defines greyd support constants and definitions.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef GREYD_DEFINED
#define GREYD_DEFINED

/**
 * Structure to encapsulate the state of the main
 * greyd process.
 */
struct Greyd_state {
    Config_T config;
    int slow_until;

    struct Con *cons;
    size_t max_files;
    size_t max_cons;
    size_t max_black;
    size_t clients;
    size_t black_clients;

    FILE *grey_out;
    FILE *trap_in;

    List_T blacklists;
};

#endif
