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

/**
 * Internal structure for containing a single blacklist entry.
 */
struct E {
    uint32_t address;
    int      black;
    int      white;
};

/**
 * The main blacklist structure. 
 */
typedef struct T *T;
struct T {
    struct E *entries;
    char     *message;
};

/**
 * Create an empty blacklist.
 */
extern T Blacklist_create(const char *message);

/**
 * Destroy a blacklist and cleanup.
 */
extern void Blacklist_destroy(T list);

/**
 * Add a range of addresses to the blacklist of the specified type.
 */
extern void Blacklist_add_range(T list, uint32_t start, uint32_t end, int type);

/**
 * "Collapse" a blacklist's entries by removing overlapping regions as well
 * as removing whitelist regions. A list of non-overlapping blacklist
 * CIDR networks is returned.
 */
extern IP_cidr *Blacklist_collapse(T list);

#undef T
#endif
