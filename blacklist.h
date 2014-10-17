/**
 * @file   blacklist.h
 * @brief  Defines the interface for the blacklist management objects.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef BLACKLIST_DEFINED
#define BLACKLIST_DEFINED

#include "ip.h"
#include "list.h"
#include <stdint.h>

#define T Blacklist_T
#define E Blacklist_entry

#define BL_TYPE_WHITE 0
#define BL_TYPE_BLACK 1

#define BLACKLIST_INIT_SIZE (1024 * 1024)

/**
 * Internal structure for containing a single blacklist entry.
 */
struct E {
    struct IP_addr address;
    struct IP_addr mask;
    int8_t         black;
    int8_t         white;
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
extern void Blacklist_destroy(T *list);

/**
 * Return 1 if the source address is in the list, otherwise return 0.
 */
extern int Blacklist_match(T list, struct IP_addr *source, sa_family_t af);

/**
 * Add a single IPv4/IPv6 formatted address to the specified blacklist's
 * list of entries.
 */
extern int Blacklist_add(T list, const char *address);

/**
 * Add a range of addresses to the blacklist of the specified type. A call to this
 * function will result in two separate entries for the start and end addresses.
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
