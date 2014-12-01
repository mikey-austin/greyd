/**
 * @file   hash.h
 * @brief  Defines an abstract hash table data structure.
 * @author Mikey Austin
 * @date   2014
 *
 * This interface defines a generic hash data structure with the capability
 * to store arbitrary values with the option of integer or NULL terminated
 * key values.
 */

#ifndef HASH_DEFINED
#define HASH_DEFINED

#include "list.h"

#define MAX_KEY_LEN 45

/**
 * A struct to contain a hash entry's key and value pair.
 */
struct Hash_entry {
    char  k[MAX_KEY_LEN + 1]; /**< Hash entry key */
    void *v;                  /**< Hash entry value */
};

/**
 * The main hash table structure.
 */
typedef struct Hash_T *Hash_T;
struct Hash_T {
    int  size;        /**< The initial hash size. */
    int  num_entries; /**< The number of set elements. */
    void (*destroy)(struct Hash_entry *entry);
    struct Hash_entry  *entries;
};

/**
 * Create a new hash table.
 */
extern Hash_T Hash_create(int size,
                          void (*destroy)(struct Hash_entry *entry));

/**
 * Destroy a hash table, freeing all elements
 */
extern void Hash_destroy(Hash_T *hash);

/**
 * Clear out all entries in the table.
 */
extern void Hash_reset(Hash_T hash);

/**
 * Insert a new element into the hash table.
 */
extern void Hash_insert(Hash_T hash, const char *key, void *value);

/**
 * Fetch an element from the hash table by the specified key.
 */
extern void *Hash_get(Hash_T hash, const char *key);

/**
 * Return a list of the set keys in the supplied hash. Note, the
 * order of the keys in the returned list is undefined.
 */
extern List_T Hash_keys(Hash_T hash);

#endif
