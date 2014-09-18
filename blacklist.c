/**
 * @file   blacklist.c
 * @brief  Implements blacklist interface and structures.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "failures.h"
#include "blacklist.h"

#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#define T Blacklist_T
#define E Blacklist_ip

#define BLACKLIST_INIT_SIZE (1024 * 1024)

extern T
Blacklist_create(const char *name, const char *message)
{
    int len;
    T blacklist;

    if((blacklist = (T) malloc(sizeof(*blacklist))) == NULL) {
        I_CRIT("Could not create blacklist");
    }

    blacklist->entries = (struct E *) calloc(BLACKLIST_INIT_SIZE + 1,
                                             sizeof(struct E));
    if(blacklist->entries == NULL) {
        I_CRIT("Could not create blacklist entries");
    }

    len = strlen(name) + 1;
    if((blacklist->name = (char *) malloc(len)) == NULL) {
        I_CRIT("could not malloc blacklist name");
    }
    sstrncpy(blacklist->name, name, len);

    len = strlen(message) + 1;
    if((blacklist->message = (char *) malloc(len)) == NULL) {
        I_CRIT("could not malloc blacklist message");
    }
    sstrncpy(blacklist->message, message, len);

    blacklist->size = BLACKLIST_INIT_SIZE + 1;
    blacklist->count = 0;

    return blacklist;
}

extern void
Blacklist_destroy(T list)
{
    if(!list) {
        return;
    }

    if(list->entries) {
        free(list->entries);
        list->entries = NULL;
    }

    if(list->name) {
        free(list->name);
        list->name = NULL;
    }

    if(list->message) {
        free(list->message);
        list->message = NULL;
    }

    free(list);
    list = NULL;
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
