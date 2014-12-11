/**
 * @file   greydb.h
 * @brief  Defines the generic greylisting database abstraction.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef GREYDB_DEFINED
#define GREYDB_DEFINED

#include "config.h"
#include "grey.h"

#include <sys/types.h>
#include <pwd.h>

#define DB_KEY_IP    1 /**< An ip address. */
#define DB_KEY_MAIL  2 /**< A valid email address. */
#define DB_KEY_TUPLE 3 /**< A greylist tuple. */

#define DB_VAL_GREY         1 /**< Grey counters data. */
#define DB_VAL_MATCH_SUFFIX 2 /**< Match suffix data. */

#define GREYDB_ERR       -1
#define GREYDB_FOUND     0
#define GREYDB_OK        1
#define GREYDB_NOT_FOUND 2

#define GREYDB_RO 1
#define GREYDB_RW 2

/**
 * Keys may be of different types depending on the type of database entry.
 */
struct DB_key {
    short type;
    union {
        char *s; /**< May contain IP or MAIL key types. */
        struct Grey_tuple gt;
    } data;
};

struct DB_val {
    short type;
    union {
        char *s;             /**< Allowed domain. */
        struct Grey_data gd; /**< Greylisting counters.h */
    } data;
};

typedef struct DB_handle_T *DB_handle_T;
typedef struct DB_itr_T *DB_itr_T;

struct DB_handle_T {
    void *driver;
    void *dbh;                /**< Driver dependent handle reference. */
    Config_T config;          /**< System configuration. */
    Config_section_T section; /**< Module configuration section. */
    struct passwd *pw;        /**< System user/group information. */

    void (*db_open)(DB_handle_T handle, int);
    void (*db_close)(DB_handle_T handle);
    int (*db_put)(DB_handle_T handle, struct DB_key *key, struct DB_val *val);
    int (*db_get)(DB_handle_T handle, struct DB_key *key, struct DB_val *val);
    int (*db_del)(DB_handle_T handle, struct DB_key *key);
    void (*db_get_itr)(DB_itr_T itr);
    int (*db_itr_next)(DB_itr_T itr, struct DB_key *key, struct DB_val *val);
    int (*db_itr_replace_curr)(DB_itr_T itr, struct DB_val *val);
    int (*db_itr_del_curr)(DB_itr_T itr);
    void (*db_itr_close)(DB_itr_T itr);
};

struct DB_itr_T {
    DB_handle_T   handle;  /**< Database handle reference. */
    void *dbi;   /**< Driver specific iterator. */
    int current; /**< Current index. */
    int size;    /**< Number of elements in the iteration. */
};

/**
 * Open a connection to the configured database based on the configuration.
 */
extern DB_handle_T DB_open(Config_T config, int flags);

/**
 * Close a database connection.
 */
extern void DB_close(DB_handle_T *handle);

/**
 * Insert a single key/value pair into the database.
 */
extern int DB_put(DB_handle_T handle, struct DB_key *key, struct DB_val *val);

/**
 * Get a single value out of the database, associated with the
 * specified key.
 */
extern int DB_get(DB_handle_T handle, struct DB_key *key, struct DB_val *val);

/**
 * Remove a record specified by the key.
 */
extern int DB_del(DB_handle_T handle, struct DB_key *key);

/**
 * Return an iterator for all database entries. If there are no entries,
 * NULL is returned.
 */
extern DB_itr_T DB_get_itr(DB_handle_T handle);

/**
 * Return the next key/value pair from the iterator.
 */
extern int DB_itr_next(DB_itr_T itr, struct DB_key *key, struct DB_val *val);

/**
 * Replace the value currently being pointed at by the iterator
 * with the supplied value.
 */
extern int DB_itr_replace_curr(DB_itr_T itr, struct DB_val *val);

/**
 * Delete the key/value pair currently being pointed at by
 * the iterator.
 */
extern int DB_itr_del_curr(DB_itr_T itr);

/**
 * Cleanup a used iterator.
 */
extern void DB_close_itr(DB_itr_T *itr);

#endif