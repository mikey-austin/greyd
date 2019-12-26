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
 * @file   hash.c
 * @brief  Implements the hash table interface.
 * @author Mikey Austin
 * @date   2014
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "failures.h"
#include "hash.h"
#include "list.h"
#include "utils.h"

/**
 * Lookup a key in the hash table and return an entry pointer. This function
 * contains the logic for linear probing conflict resolution.
 *
 * An entry with a NULL value indicates that the entry is not being used.
 */
static struct Hash_entry* Hash_find_entry(Hash_T hash, const char* key);

/**
 * Resize the list of entries if the number of entries equals the configured
 * size.
 */
static void Hash_resize(Hash_T hash, int new_size);

/**
 * The hash function to map a key to an index in the array of hash entries.
 */
static unsigned int Hash_lookup(const char* key);

/**
 * Create and initialize the hash's array of entries.
 */
static void Hash_create_entries(Hash_T hash);

extern Hash_T
Hash_create(int size, void (*destroy)(struct Hash_entry* entry))
{
    int i;
    Hash_T new;

    if ((new = malloc(sizeof(*new))) == NULL)
        i_critical("malloc: %s", strerror(errno));

    new->size = size;
    new->destroy = destroy;

    /* Leave the entries uninitialized until the first insert. */
    new->entries = NULL;
    new->num_entries = 0;

    return new;
}

extern void
Hash_destroy(Hash_T* hash)
{
    int i;

    if (hash == NULL || *hash == NULL) {
        return;
    }

    if ((*hash)->destroy && (*hash)->num_entries > 0) {
        for (i = 0; i < (*hash)->size; i++) {
            (*hash)->destroy(((*hash)->entries + i));
        }
    }

    if (*hash) {
        if ((*hash)->entries) {
            free((*hash)->entries);
            (*hash)->entries = NULL;
        }
        free(*hash);
        *hash = NULL;
    }
}

extern void
Hash_reset(Hash_T hash)
{
    int i;

    if (hash->num_entries == 0)
        return;

    for (i = 0; i < hash->size; i++) {
        hash->destroy((hash->entries + i));
        hash->entries[i].v = NULL;
    }

    /* Reset the number of entries counter. */
    hash->num_entries = 0;
}

extern void
Hash_insert(Hash_T hash, const char* key, void* value)
{
    struct Hash_entry* entry;

    if (hash->entries == NULL) {
        Hash_create_entries(hash);
    } else if (hash->num_entries >= hash->size) {
        Hash_resize(hash, (2 * hash->size));
    }

    entry = Hash_find_entry(hash, key);
    if (entry->v != NULL) {
        /* An entry exists, clear contents before overwriting. */
        hash->destroy(entry);
    } else {
        /* As nothing was over written, increment the number of entries. */
        hash->num_entries++;
    }

    /* Setup the new entry. */
    sstrncpy(entry->k, key, MAX_KEY_LEN + 1);
    entry->v = value;
}

extern void* Hash_get(Hash_T hash, const char* key)
{
    struct Hash_entry* entry;

    if (hash->entries == NULL)
        return NULL;

    entry = Hash_find_entry(hash, key);
    return entry->v;
}

extern void
Hash_delete(Hash_T hash, const char* key)
{
    struct Hash_entry* entry;

    if (hash->entries == NULL)
        return;

    entry = Hash_find_entry(hash, key);
    if (entry->v != NULL) {
        hash->destroy(entry);
        *(entry->k) = '\0';
        entry->v = NULL;
        hash->num_entries--;
    }
}

extern List_T
Hash_keys(Hash_T hash)
{
    List_T keys = NULL;
    struct Hash_entry* entry;
    int i;

    if (hash && hash->num_entries > 0) {
        keys = List_create(NULL);
        for (i = 0; i < hash->size; i++) {
            entry = hash->entries + i;
            if (entry->k && entry->v != NULL) {
                List_insert_after(keys, entry->k);
            }
        }
    }

    return keys;
}

static struct Hash_entry* Hash_find_entry(Hash_T hash, const char* key)
{
    unsigned int i, j;
    struct Hash_entry* curr;

    i = j = (Hash_lookup(key) % hash->size);
    do {
        curr = hash->entries + i;
        if ((strcmp(key, curr->k) == 0) || (curr->v == NULL))
            break;

        i = ((i + 1) % hash->size);
    } while (i != j);

    return curr;
}

static void
Hash_resize(Hash_T hash, int new_size)
{
    int i, old_size = hash->size;
    struct Hash_entry* old_entries = hash->entries;

    if (new_size <= hash->size) {
        i_warning("Refusing to resize a hash of %d elements to %d",
            hash->size, new_size);
        return;
    }

    /*
     * We must create a whole new larger area and re-hash each element as
     * the hash function depends on the size, which is changing. This is
     * not very efficient for large hashes, so best to choose an
     * appropriate starting size.
     */
    hash->entries = (struct Hash_entry*)calloc(
        new_size, sizeof(*(hash->entries)));

    if (!hash->entries)
        i_critical("Could not resize hash entries of size %d to %d",
            hash->size, new_size);

    /* Re-initialize the hash entries. */
    hash->size = new_size;
    hash->num_entries = 0;

    /* For each non-NULL entry, re-hash into the new entries array. */
    for (i = 0; i < old_size; i++) {
        if (old_entries[i].v != NULL) {
            Hash_insert(hash, old_entries[i].k, old_entries[i].v);
        }
    }

    /* Cleanup the old entries. */
    free(old_entries);
    old_entries = NULL;
}

static void
Hash_create_entries(Hash_T hash)
{
    int i;

    hash->num_entries = 0;
    hash->entries = (struct Hash_entry*)calloc(
        hash->size, sizeof(*(hash->entries)));
    if (!hash->entries)
        i_critical("Could not allocate hash entries of size %d", hash->size);
}

/*
 * Use the djb2 string hash function.
 */
static unsigned int
Hash_lookup(const char* key)
{
    int c;
    unsigned int hash = 5381;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}
