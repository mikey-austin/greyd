/**
 * @file   hash.c
 * @brief  Implements the hash table interface.
 * @author Mikey Austin
 * @date   2014
 */
 
#include "hash.h"
#include "failures.h"

#include <stdlib.h>
#include <string.h>

#define T Hash_T
#define E Hash_entry

/**
 * Lookup a key in the hash table and return an entry pointer. This function
 * contains the logic for linear probing conflict resolution.
 *
 * An entry with a NULL value indicates that the entry is not being used.
 */
static struct E *Hash_find_entry(T hash, const char *key);

extern T
Hash_create(int size, int (*lookup)(const char *key),
    int (*cmp)(const char *a, const char *b), void (*destroy)(struct E *entry))
{
    int i;
    T new;

    new = (T) malloc(sizeof(*new));
    if(!new) {
        I_CRIT("Could not allocate hash");
    }

    new->size    = size;
    new->lookup  = lookup;
    new->cmp     = cmp;
    new->destroy = destroy;

    new->entries = (struct E*) malloc(sizeof(*(new->entries)) * size);
    if(!new->entries) {
        free(new);
        I_CRIT("Could not allocate hash entries of size %d", size);
    }

    /* Initialise the entries to NULL & indexes to -1. */
    for(i=0; i < new->size; i++) {
        new->entries[i].v = NULL;
    }

    return new;
}

extern void
Hash_destroy(T hash)
{
    int i;

    for(i=0; i < hash->size; i++) {
        hash->destroy((hash->entries + i));
    }

    if(hash && hash->entries) {
        free(hash->entries); hash->entries = NULL;
        free(hash); hash = NULL;
    } else {
        I_ERR("Tried to free NULL hash (hash %d, entries %d)",
              hash, (hash ? hash->entries : 0x0));
    }
}

extern void
Hash_reset(T hash)
{
    int i;

    for(i=0; i < hash->size; i++) {
        hash->destroy((hash->entries + i));
        hash->entries[i].v = NULL;
    }
}

extern void
Hash_insert(T hash, const char *key, void *value)
{
    struct E *entry;
    
    entry = Hash_find_entry(hash, key);
    if(entry->v != NULL) {
        /* An entry exists, clear contents before overwriting. */
        hash->destroy(entry);
    }

    /* Setup the new entry. */
    strncpy(entry->k, key, MAX_KEY_LEN + 1);
    entry->k[MAX_KEY_LEN] = '\0';
    entry->v = value;
}

extern void
*Hash_get(T hash, char *key)
{
    struct E *entry;
    entry = Hash_find_entry(hash, key);
    return entry->v;
}

static struct E
*Hash_find_entry(T hash, const char *key)
{
    int i, j;
    struct E *curr;

    i = j = (hash->lookup(key) % hash->size);
    do {
        curr = hash->entries + i;
        if((hash->cmp(key, curr->k) == 0) || (curr->v == NULL))
            break;

        i = ((i + 1) % hash->size);
    }
    while(i != j);

    return curr;
}

#undef T
#undef E
#undef K
