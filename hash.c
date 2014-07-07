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

/**
 * Resize the list of entries if the number of entries equals the configured
 * size.
 */
static void Hash_resize(T hash, int new_size);

/**
 * Initialise a hash entry safely.
 */
static void Hash_init_entry(struct E *entry);

/**
 * The hash function to map a key to an index in the array of hash entries.
 */
static int Hash_lookup(const char *key);

extern T
Hash_create(int size, void (*destroy)(struct E *entry))
{
    int i;
    T new;

    new = (T) malloc(sizeof(*new));
    if(!new) {
        I_CRIT("Could not allocate hash");
    }

    new->size        = size;
    new->num_entries = 0;
    new->destroy     = destroy;

    new->entries = (struct E*) malloc(sizeof(*(new->entries)) * size);
    if(!new->entries) {
        free(new);
        I_CRIT("Could not allocate hash entries of size %d", size);
    }

    /* Initialise the entries to NULL. */
    for(i=0; i < new->size; i++)
        Hash_init_entry(new->entries + i);

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
        free(hash->entries);
        free(hash);
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

    /* Reset the number of entries counter. */
    hash->num_entries = 0;
}

extern void
Hash_insert(T hash, const char *key, void *value)
{
    struct E *entry;

    /* Check if there is space and resize if required. */
    if(hash->num_entries >= hash->size) {
        Hash_resize(hash, (2 * hash->size));
    } 

    entry = Hash_find_entry(hash, key);
    if(entry->v != NULL) {
        /* An entry exists, clear contents before overwriting. */
        hash->destroy(entry);
    } else {
        /* As nothing was over written, increment the number of entries. */
        hash->num_entries++;
    }

    /* Setup the new entry. */
    strncpy(entry->k, key, MAX_KEY_LEN + 1);
    entry->k[MAX_KEY_LEN] = '\0';
    entry->v = value;
}

extern void
*Hash_get(T hash, const char *key)
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

    i = j = (Hash_lookup(key) % hash->size);
    do {
        curr = hash->entries + i;
        if((strcmp(key, curr->k) == 0) || (curr->v == NULL))
            break;

        i = ((i + 1) % hash->size);
    }
    while(i != j);

    return curr;
}

static void
Hash_resize(T hash, int new_size)
{
    int i, old_size = hash->size;
    struct E *old_entries = hash->entries;

    if(new_size <= hash->size) {
        I_WARN("Refusing to resize a hash of %d elements to %d", hash->size, new_size);
        return;
    }

    /*
     * We must create a whole new larger area and re-hash each element as the hash function
     * depends on the size, which is changing. This is not very efficient for large hashes,
     * so best to choose an appropriate starting size.
     */
    hash->entries = (struct E*) malloc(sizeof(*(hash->entries)) * new_size);

    if(!hash->entries) {
        I_CRIT("Could not resize hash entries of size %d to %d", hash->size, new_size);
    }

    /* Re-initialize the hash entries. */
    hash->size = new_size;
    hash->num_entries = 0;
    for(i = 0; i < hash->size; i++)
        Hash_init_entry(hash->entries + i);

    /* For each non-NULL entry, re-hash into the new entries array. */
    for(i = 0; i < old_size; i++) {
        if(old_entries[i].v != NULL) {
            Hash_insert(hash, old_entries[i].k, old_entries[i].v);
        }
    }

    /* Cleanup the old entries. */
    free(old_entries);
}

static void
Hash_init_entry(struct E *entry)
{
    /* Ensure that an empty key contains a null byte. */
    *(entry->k) = '\0';
    entry->v    = NULL;
}

static int
Hash_lookup(const char *key)
{
    return strlen(key);
}

#undef T
#undef E
#undef K
