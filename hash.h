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

#define T Hash_T
#define E Hash_entry

#define MAX_KEY_LEN 45

/**
 * A struct to contain a hash entry's key and value pair.
 */
struct E {
    char  k[MAX_KEY_LEN + 1]; /**< Hash entry key */
    void *v;                  /**< Hash entry value */
};

/**
 * The main hash table structure.
 */
typedef struct T *T;
struct T {
    int        size;        /**< The initial hash size. */
    int        num_entries; /**< The number of set elements. */
    int      (*lookup)(const char *key);
    void     (*destroy)(struct E *entry);
    struct E  *entries;
};

/**
 * Create a new hash table.
 */
extern T Hash_create(int    size,
                     int  (*lookup)(const char *key),
                     void (*destroy)(struct E *entry));

/**
 * Destroy a hash table, freeing all elements
 */
extern void Hash_destroy(T hash);

/**
 * Clear out all entries in the table.
 */
extern void Hash_reset(T hash);

/**
 * Insert a new element into the hash table.
 */
extern void Hash_insert(T hash, const char *key, void *value);

/**
 * Fetch an element from the hash table by the specified key.
 */
extern void *Hash_get(T hash, char *key);

#undef T
#undef E
#undef K
#endif
