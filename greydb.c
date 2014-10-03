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
DB_open(Config_T config)
{
    H handle;
    void *mod_handle;
    void (*db_open)(H handle);
    Config_section_T section;
    char *user;

    /* Setup the db handle. */
    if((handle = (H) malloc(sizeof(*handle))) == NULL) {
        I_CRIT("Could not create db handle");
    }

    if((section = Config_get_section(config, "database")) == NULL) {
        I_CRIT("Could not find database configuration");
    }

    handle->config  = config;
    handle->section = section;

    section = Config_get_section(config, CONFIG_DEFAULT_SECTION);
    if((user = Config_section_get_str(section, "user", NULL)) != NULL) {
        if((handle->pw = getpwnam(user)) == NULL) {
            I_CRIT("No such user %s", user);
        }
    }
    else {
        handle->pw = NULL;
    }
    
    mod_handle = Mod_open(handle->section, "db");
    db_open = (void (*)(H)) Mod_get(mod_handle, "Mod_db_open");
    (*db_open)(handle);
    Mod_close(mod_handle);

    return handle;
}

extern void
DB_close(H *handle)
{
    void *mod_handle;
    void (*db_close)(H handle);

    if(handle == NULL || *handle == NULL) {
        return;
    }

    mod_handle = Mod_open((*handle)->section, "db");
    db_close = (void (*)(H)) Mod_get(mod_handle, "Mod_db_close");
    (*db_close)(*handle);
    Mod_close(mod_handle);

    free(*handle);
    *handle = NULL;
}

extern int
DB_put(H handle, struct K *key, struct V *val)
{
    void *mod_handle;
    int (*db_put)(H handle, struct K *key, struct V *val);
    int ret;

    mod_handle = Mod_open(handle->section, "db");
    db_put = (int (*)(H, struct K *, struct V *))
        Mod_get(mod_handle, "Mod_db_put");
    ret = (*db_put)(handle, key, val);
    Mod_close(mod_handle);

    return ret;
}

extern int
DB_get(H handle, struct K *key, struct V *val)
{
    void *mod_handle;
    int (*db_get)(H handle, struct K *key, struct V *val);
    int ret;

    mod_handle = Mod_open(handle->section, "db");
    db_get = (int (*)(H, struct K *, struct V *))
        Mod_get(mod_handle, "Mod_db_get");
    ret = (*db_get)(handle, key, val);
    Mod_close(mod_handle);

    return ret;
}

extern int
DB_del(H handle, struct K *key)
{
    void *mod_handle;
    int (*db_del)(H handle, struct K *key);
    int ret;

    mod_handle = Mod_open(handle->section, "db");
    db_del = (int (*)(H, struct K *))
        Mod_get(mod_handle, "Mod_db_del");
    ret = (*db_del)(handle, key);
    Mod_close(mod_handle);

    return ret;
}

extern I
DB_get_itr(H handle)
{
    void *mod_handle;
    void (*db_get_itr)(I itr);
    I itr;

    /* Setup the iterator. */
    if((itr = (I) malloc(sizeof(*itr))) == NULL) {
        I_CRIT("Could not create iterator");
    }

    itr->handle  = handle;
    itr->current = -1;
    itr->size    = 0;

    mod_handle = Mod_open(handle->section, "db");
    db_get_itr = (void (*)(I)) Mod_get(mod_handle, "Mod_db_get_itr");
    (*db_get_itr)(itr);
    Mod_close(mod_handle);

    return itr;
}

extern int
DB_itr_next(I itr, struct K *key, struct V *val)
{
    void *mod_handle;
    int (*db_itr_next)(I itr, struct K *key, struct V *val);
    int ret;

    mod_handle = Mod_open(itr->handle->section, "db");
    db_itr_next = (int (*)(I, struct K *, struct V *))
        Mod_get(mod_handle, "Mod_db_itr_next");
    ret = (*db_itr_next)(itr, key, val);
    Mod_close(mod_handle);

    return ret;
}

extern void
DB_close_itr(I *itr)
{
    void *mod_handle;
    void (*db_itr_close)(I itr);

    if(itr == NULL || *itr == NULL) {
        return;
    }

    mod_handle = Mod_open((*itr)->handle->section, "db");
    db_itr_close = (void (*)(I))
        Mod_get(mod_handle, "Mod_db_itr_close");
    (*db_itr_close)(*itr);
    Mod_close(mod_handle);

    free(*itr);
    *itr = NULL;
}

#undef K
#undef V
#undef H
#undef I
