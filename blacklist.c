/**
 * @file   blacklist.c
 * @brief  Implements blacklist interface and structures.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "blacklist.h"

#include <stdlib.h>
#include <arpa/inet.h>

#define T Blacklist_T
#define E Blacklist_ip

#define BLACKLIST_INIT_SIZE (1024 * 1024)

extern T
Blacklist_create(const char *message)
{
    return NULL;
}

extern void
Blacklist_destroy(T list)
{
}

extern void
Blacklist_add_range(T list, u_int32_t start, u_int32_t end, int type)
{
    int i;

    if(list->count >= (list->size - 2)) {
        list->entries = realloc(
            list->entries, list->size + BLACKLIST_INIT_SIZE + 1);

        if(list->entries == NULL) {
            I_CRIT("realloc failed");
        }

        list->size += BLACKLIST_INIT_SIZE + 1;
    }

    /* Reserve room for the pair. */
    list->count += 2;
    i = list->count - 1; /* Start entry. */

    list->entries[i].address = ntohl(start);
    list->entries[i + 1].address = ntohl(end);

    if(type == BL_TYPE_WHITE) {
        list->entries[i].black = 0;
        list->entries[i].white = 1;
        list->entries[i + 1].black = 0;
        list->entries[i + 1].white = -1;
    }
    else {
        list->entries[i].black = 1;
        list->entries[i].white = 0;
        list->entries[i + 1].black = -1;
        list->entries[i + 1].white = 0;
    }
}

extern struct IP_cidr
*Blacklist_collapse(T list)
{
    return NULL;
}

#undef T
#undef E
