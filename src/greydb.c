/**
 * @file   greydb.c
 * @brief  Implements the generic greylisting database abstraction.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "greydb.h"
#include "mod.h"
#include "constants.h"

#include <stdlib.h>

extern DB_handle_T
DB_open(Config_T config, int flags)
{
    DB_handle_T handle;
    Config_section_T section;
    char *user;

    /* Setup the db handle. */
    if((handle = malloc(sizeof(*handle))) == NULL) {
        i_critical("Could not create db handle");
    }

    if((section = Config_get_section(config, "database")) == NULL) {
        i_critical("Could not find database configuration");
    }

    handle->config  = config;
    if((user = Config_get_str(config, "user", "grey", GREYD_DB_USER)) != NULL) {
        if((handle->pw = getpwnam(user)) == NULL) {
            i_critical("No such user %s", user);
        }
    }
    else {
        handle->pw = NULL;
    }

    /* Open the configured driver and extract all required symbols. */
    handle->driver = Mod_open(section, "db");

    handle->db_open = (void (*)(DB_handle_T, int))
        Mod_get(handle->driver, "Mod_db_open");
    handle->db_close = (void (*)(DB_handle_T))
        Mod_get(handle->driver, "Mod_db_close");
    handle->db_put = (int (*)(DB_handle_T, struct DB_key *, struct DB_val *))
        Mod_get(handle->driver, "Mod_db_put");
    handle->db_get = (int (*)(DB_handle_T, struct DB_key *, struct DB_val *))
        Mod_get(handle->driver, "Mod_db_get");
    handle->db_del = (int (*)(DB_handle_T, struct DB_key *))
        Mod_get(handle->driver, "Mod_db_del");
    handle->db_get_itr = (void (*)(DB_itr_T))
        Mod_get(handle->driver, "Mod_db_get_itr");
    handle->db_itr_next = (int (*)(DB_itr_T, struct DB_key *, struct DB_val *))
        Mod_get(handle->driver, "Mod_db_itr_next");
    handle->db_itr_replace_curr = (int (*)(DB_itr_T, struct DB_val *))
        Mod_get(handle->driver, "Mod_db_itr_replace_curr");
    handle->db_itr_del_curr = (int (*)(DB_itr_T))
        Mod_get(handle->driver, "Mod_db_itr_del_curr");
    handle->db_itr_close = (void (*)(DB_itr_T))
        Mod_get(handle->driver, "Mod_db_itr_close");

    /* Initialize the database driver. */
    handle->db_open(handle, flags);

    return handle;
}

extern void
DB_close(DB_handle_T *handle)
{
    if(handle == NULL || *handle == NULL) {
        return;
    }

    (*handle)->db_close(*handle);
    Mod_close((*handle)->driver);
    free(*handle);
    *handle = NULL;
}

extern int
DB_put(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    return handle->db_put(handle, key, val);
}

extern int
DB_get(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    return handle->db_get(handle, key, val);
}

extern int
DB_del(DB_handle_T handle, struct DB_key *key)
{
    return handle->db_del(handle, key);
}

extern DB_itr_T
DB_get_itr(DB_handle_T handle)
{
    DB_itr_T itr;

    /* Setup the iterator. */
    if((itr = malloc(sizeof(*itr))) == NULL) {
        i_critical("Could not create iterator");
    }

    itr->handle  = handle;
    itr->current = -1;
    itr->size    = 0;

    handle->db_get_itr(itr);

    return itr;
}

extern int
DB_itr_next(DB_itr_T itr, struct DB_key *key, struct DB_val *val)
{
    return itr->handle->db_itr_next(itr, key, val);
}

extern int
DB_itr_replace_curr(DB_itr_T itr, struct DB_val *val)
{
    return itr->handle->db_itr_replace_curr(itr, val);
}

extern int
DB_itr_del_curr(DB_itr_T itr)
{
    return itr->handle->db_itr_del_curr(itr);
}

extern void
DB_close_itr(DB_itr_T *itr)
{
    if(itr == NULL || *itr == NULL) {
        return;
    }

    (*itr)->handle->db_itr_close(*itr);
    free(*itr);
    *itr = NULL;
}
