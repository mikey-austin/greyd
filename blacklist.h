/**
 * @file   blacklist.h
 * @brief  Defines the interface for the blacklist management objects.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef BLACKLIST_DEFINED
#define BLACKLIST_DEFINED

#include "ip.h"
#include <stdint.h>

#define T Blacklist_T
#define E Blacklist_ip

#define BL_TYPE_WHITE 0
#define BL_TYPE_BLACK 1

#define BLACKLIST_INIT_SIZE (1024 * 1024)

/**
 * Internal structure for containing a single blacklist entry.
 */
struct E {
    u_int32_t address;
    int8_t    black;
    int8_t    white;
};

/**
 * The main blacklist structure. 
 */
typedef struct T *T;
struct T {
    char     *name;
    char     *message;
    struct E *entries;
    size_t   size;
    size_t   count;
};

/**
 * Create an empty blacklist.
 */
extern T Blacklist_create(const char *name, const char *message);

/**
 * Destroy a blacklist and cleanup.
 */
extern void Blacklist_destroy(T list);

/**
 * Add a range of addresses to the blacklist of the specified type.
 */
extern void Blacklist_add_range(T list, u_int32_t start, u_int32_t end, int type);

/**
 * "Collapse" a blacklist's entries by removing overlapping regions as well
 * as removing whitelist regions. A list of non-overlapping blacklist
 * CIDR networks is returned, suitable for feeding in printable form to
 * a firewall or greyd.
 */
extern List_T Blacklist_collapse(T blacklist);

#undef T
#undef E
#endif
