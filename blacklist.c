/**
 * @file   blacklist.c
 * @brief  Implements blacklist interface and structures.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "failures.h"
#include "blacklist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T Blacklist_T
#define E Blacklist_entry

static int cmp_ipv4_entry(const void *a, const void *b);
static void cidr_destroy(void *cidr);
static void grow_entries(T list);

extern T
Blacklist_create(const char *name, const char *message)
{
    int len;
    T blacklist;

    if((blacklist = malloc(sizeof(*blacklist))) == NULL) {
        I_CRIT("Could not create blacklist");
    }
    blacklist->entries = (struct E *) calloc(BLACKLIST_INIT_SIZE,
                                             sizeof(struct E));
    if(blacklist->entries == NULL) {
        I_CRIT("Could not create blacklist entries");
    }

    len = strlen(name) + 1;
    if((blacklist->name = malloc(len)) == NULL) {
        I_CRIT("could not malloc blacklist name");
    }
    sstrncpy(blacklist->name, name, len);

    len = strlen(message) + 1;
    if((blacklist->message = malloc(len)) == NULL) {
        I_CRIT("could not malloc blacklist message");
    }
    sstrncpy(blacklist->message, message, len);

    blacklist->size = BLACKLIST_INIT_SIZE;
    blacklist->count = 0;

    return blacklist;
}

extern void
Blacklist_destroy(T *list)
{
    if(list == NULL || *list == NULL) {
        return;
    }

    if((*list)->entries) {
        free((*list)->entries);
        (*list)->entries = NULL;
    }

    if((*list)->name) {
        free((*list)->name);
        (*list)->name = NULL;
    }

    if((*list)->message) {
        free((*list)->message);
        (*list)->message = NULL;
    }

    free(*list);
    *list = NULL;
}

extern int
Blacklist_match(T list, struct IP_addr *source, sa_family_t af)
{
    int i;
    struct IP_addr *a, *m;

    for(i = 0; i < list->count; i++) {
        a = &(list->entries[i].address);
        m = &(list->entries[i].mask);

        if(IP_match_addr(a, m, source, af) > 0) {
            return 1;
        }
    }

    return 0;
}

extern int
Blacklist_add(T list, const char *address)
{
    int ret, maskbits, af, i, j;
    char parsed[INET6_ADDRSTRLEN];
    struct IP_addr *m, *n;

    grow_entries(list);
    i = list->count++;
    n = &(list->entries[i].address);
    m = &(list->entries[i].mask);

    ret = sscanf(address, "%39[^/]/%u", parsed, &maskbits);
    if(ret != 2 || maskbits == 0 || maskbits > IP_MAX_MASKBITS) {
        goto parse_error;
    }

    af = (strchr(parsed, ':') != NULL ? AF_INET6 : AF_INET);
    if(af == AF_INET && maskbits > IP_MAX_MASKBITS_V4) {
        goto parse_error;
    }

    if((ret = inet_pton(af, parsed, n)) != 1) {
        goto parse_error;
    }
    else {
        //I_DEBUG("Added %s/%u\n", parsed, maskbits);
    }

    for(i = 0, j = 0; i < 4; i++)
        m->addr32[i] = 0;

    while(maskbits >= 32) {
        m->addr32[j++] = 0xffffffff;
        maskbits -= 32;
    }

    for(i = 31; i > (31 - maskbits); --i)
        m->addr32[j] |= (1 << i);

    if(maskbits)
        m->addr32[j] = htonl(m->addr32[j]);

    /* Mask off address bits that won't ever be used. */
    for(i = 0; i < 4; i++)
        n->addr32[i] = n->addr32[i] & m->addr32[i];

    return 0;

parse_error:
    I_DEBUG("Error parsing address %s", address);
    return -1;
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

    grow_entries(list);

    if(list->entries) {
        /* Reserve room for the pair. */
        i = list->count;
        list->count += 2;

        list->entries[i].address.v4.s_addr = start;
        list->entries[i + 1].address.v4.s_addr = end;

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
}

extern List_T
Blacklist_collapse(T blacklist)
{
    int i, bs = 0, ws = 0, state = 0, laststate;
    u_int32_t addr, bstart = 0;
    List_T cidrs;

    if(blacklist->count == 0)
        return NULL;

    qsort(blacklist->entries, blacklist->count, sizeof(struct E),
          cmp_ipv4_entry);
    cidrs = List_create(cidr_destroy);

    for(i = 0; i < blacklist->count; ) {
        laststate = state;
        addr = blacklist->entries[i].address.v4.s_addr;

        do {
            bs += blacklist->entries[i].black;
            ws += blacklist->entries[i].white;
            i++;
        } while(blacklist->entries[i].address.v4.s_addr == addr);

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
    }

    return cidrs;
}

static void
grow_entries(T list)
{
    if(list->count >= (list->size - 2)) {
        list->entries = realloc(
            list->entries, list->size + BLACKLIST_INIT_SIZE);

        if(list->entries == NULL) {
            I_CRIT("realloc failed");
        }

        list->size += BLACKLIST_INIT_SIZE;
    }
}

static int
cmp_ipv4_entry(const void *a, const void *b)
{
    if(((struct E *) a)->address.v4.s_addr
       > ((struct E *) b)->address.v4.s_addr)
    {
        return 1;
    }

    if(((struct E *) a)->address.v4.s_addr
       < ((struct E *) b)->address.v4.s_addr)
    {
        return -1;
    }

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
