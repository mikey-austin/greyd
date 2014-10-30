/**
 * @file   greydb.c
 * @brief  Implements the generic greylisting database abstraction.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "greydb.h"
#include "mod.h"

#include <stdlib.h>

#define H DB_handle_T
#define I DB_itr_T
#define K DB_key
#define V DB_val

extern H
DB_open(Config_T config, int flags)
{
    H handle;
    Config_section_T section;
    char *user;

    /* Setup the db handle. */
    if((handle = malloc(sizeof(*handle))) == NULL) {
        I_CRIT("Could not create db handle");
    }

    if((section = Config_get_section(config, "database")) == NULL) {
        I_CRIT("Could not find database configuration");
    }

    handle->config  = config;
    handle->section = section;

    if((user = Config_section_get_str(section, "user", NULL)) != NULL) {
        if((handle->pw = getpwnam(user)) == NULL) {
            I_CRIT("No such user %s", user);
        }
    }
    else {
        handle->pw = NULL;
    }

    /* Open the configured driver and extract all required symbols. */
    handle->driver = Mod_open(handle->section, "db");

    handle->db_open = (void (*)(H, int)) Mod_get(handle->driver, "Mod_db_open");
    handle->db_close = (void (*)(H)) Mod_get(handle->driver, "Mod_db_close");
    handle->db_put = (int (*)(H, struct K *, struct V *))
        Mod_get(handle->driver, "Mod_db_put");
    handle->db_get = (int (*)(H, struct K *, struct V *))
        Mod_get(handle->driver, "Mod_db_get");
    handle->db_del = (int (*)(H, struct K *))
        Mod_get(handle->driver, "Mod_db_del");
    handle->db_get_itr = (void (*)(I))
        Mod_get(handle->driver, "Mod_db_get_itr");
    handle->db_itr_next = (int (*)(I, struct K *, struct V *))
        Mod_get(handle->driver, "Mod_db_itr_next");
    handle->db_itr_replace_curr = (int (*)(I, struct V *))
        Mod_get(handle->driver, "Mod_db_itr_replace_curr");
    handle->db_itr_del_curr = (int (*)(I))
        Mod_get(handle->driver, "Mod_db_itr_del_curr");
    handle->db_itr_close = (void (*)(I))
        Mod_get(handle->driver, "Mod_db_itr_close");

    /* Initialize the database driver. */
    handle->db_open(handle, flags);

    return handle;
}

extern void
DB_close(H *handle)
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
DB_put(H handle, struct K *key, struct V *val)
{
    return handle->db_put(handle, key, val);
}

extern int
DB_get(H handle, struct K *key, struct V *val)
{
    return handle->db_get(handle, key, val);
}

extern int
DB_del(H handle, struct K *key)
{
    return handle->db_del(handle, key);
}

extern I
DB_get_itr(H handle)
{
    I itr;

    /* Setup the iterator. */
    if((itr = malloc(sizeof(*itr))) == NULL) {
        I_CRIT("Could not create iterator");
    }

    itr->handle  = handle;
    itr->current = -1;
    itr->size    = 0;

    handle->db_get_itr(itr);

    return itr;
}

extern int
DB_itr_next(I itr, struct K *key, struct V *val)
{
    return itr->handle->db_itr_next(itr, key, val);
}

extern int
DB_itr_replace_curr(I itr, struct V *val)
{
    return itr->handle->db_itr_replace_curr(itr, val);
}

extern int
DB_itr_del_curr(I itr)
{
    return itr->handle->db_itr_del_curr(itr);
}

extern void
DB_close_itr(I *itr)
{
    if(itr == NULL || *itr == NULL) {
        return;
    }

    (*itr)->handle->db_itr_close(*itr);
    free(*itr);
    *itr = NULL;
}

#undef K
#undef V
#undef H
#undef I
