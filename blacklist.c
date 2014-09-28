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

static int cmp_entry(const void *a, const void *b);
static void cidr_destroy(void *cidr);

extern T
Blacklist_create(const char *name, const char *message)
{
    int len;
    T blacklist;

    if((blacklist = (T) malloc(sizeof(*blacklist))) == NULL) {
        I_CRIT("Could not create blacklist");
    }

    blacklist->entries = (struct E *) calloc(BLACKLIST_INIT_SIZE,
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

    blacklist->size = BLACKLIST_INIT_SIZE;
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

    /*
     * If the start address is greater than the end address, ignore entry.
     */
    if(start > end)
        return;

    if(list->count >= (list->size - 2)) {
        list->entries = realloc(
            list->entries, list->size + BLACKLIST_INIT_SIZE);

        if(list->entries == NULL) {
            I_CRIT("realloc failed");
        }

        list->size += BLACKLIST_INIT_SIZE;
    }

    /* Reserve room for the pair. */
    i = list->count;
    list->count += 2;

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

extern List_T
Blacklist_collapse(T blacklist)
{
    int i, bs = 0, ws = 0, state = 0, laststate;
    u_int32_t addr, bstart = 0;
    List_T cidrs;

    if(blacklist->count == 0)
        return NULL;

    qsort(blacklist->entries, blacklist->count, sizeof(struct E), cmp_entry);
    cidrs = List_create(cidr_destroy);

    for(i = 0; i < blacklist->count; ) {
		laststate = state;
		addr = blacklist->entries[i].address;

		do {
			bs += blacklist->entries[i].black;
			ws += blacklist->entries[i].white;
			i++;
		} while(blacklist->entries[i].address == addr);

		if(state == 1 && bs == 0)
			state = 0;
		else if(state == 0 && bs > 0)
			state = 1;

		if(ws > 0)
			state = 0;

		if(laststate == 0 && state == 1) {
			/*
             * This state transition marks the start of a blacklist region.
             */
			bstart = addr;
		}

		if(laststate == 1 && state == 0) {
			/*
             * We are at the end of a blacklist region, convert the range
             * into CIDR format.
             */
            IP_range_to_cidr_list(cidrs, bstart, (addr - 1));
		}

		laststate = state;
    }

    return cidrs;
}

static int
cmp_entry(const void *a, const void *b)
{
    if(((struct E *) a)->address > ((struct E *) b)->address)
        return 1;

    if(((struct E *) a)->address < ((struct E *) b)->address)
        return -1;

    return 0;
}

static void
cidr_destroy(void *cidr)
{
    if(cidr) {
        free(cidr);
        cidr = NULL;
    }
}

#undef T
#undef E
