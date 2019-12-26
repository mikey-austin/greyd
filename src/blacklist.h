/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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
#include "trie.h"
#include <stdint.h>

#define BL_TYPE_WHITE 0
#define BL_TYPE_BLACK 1

#define BL_STORAGE_LIST 0
#define BL_STORAGE_TRIE 1

#define BLACKLIST_INIT_SIZE (1024 * 1024)

/**
 * Internal structure for containing a single blacklist entry.
 */
struct Blacklist_entry {
    struct IP_addr address;
    struct IP_addr mask;
    int8_t black;
    int8_t white;
};

struct Blacklist_trie_entry {
    sa_family_t af;
    struct IP_addr address;
    struct IP_addr mask;
};

/**
 * The main blacklist structure.
 */
typedef struct Blacklist_T* Blacklist_T;
struct Blacklist_T {
    char* name;
    char* message;
    size_t size;
    size_t count;
    int type;
    struct Trie* trie;
    struct Blacklist_entry* entries;
};

/**
 * Create an empty blacklist.
 */
extern Blacklist_T Blacklist_create(const char* name, const char* message, int flags);

/**
 * Destroy a blacklist and cleanup.
 */
extern void Blacklist_destroy(Blacklist_T* list);

/**
 * Return 1 if the source address is in the list, otherwise return 0.
 */
extern int Blacklist_match(Blacklist_T list, struct IP_addr* source,
    sa_family_t af);

/**
 * Add a single IPv4/IPv6 formatted address to the specified blacklist's
 * list of entries.
 */
extern int Blacklist_add(Blacklist_T list, const char* address);

/**
 * Add a range of addresses to the blacklist of the specified type. A
 * call to this function will result in two separate entries for the
 * start and end addresses.
 */
extern void Blacklist_add_range(Blacklist_T list, u_int32_t start,
    u_int32_t end, int type);

/**
 * "Collapse" a blacklist's entries by removing overlapping regions as well
 * as removing whitelist regions. A list of non-overlapping blacklist
 * CIDR networks is returned, suitable for feeding in printable form to
 * a firewall or greyd.
 */
extern List_T Blacklist_collapse(Blacklist_T blacklist);

#endif
