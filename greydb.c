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
#define K DB_key_T
#define V DB_val_T
#define I DB_itr_T

extern H
DB_open(Config_section_T section)
{
    H handle;
    void *mod_handle;
    void (*db_open)(H handle);

    /* Setup the db handle. */
    if((handle = (H) malloc(sizeof(*handle))) == NULL) {
        I_CRIT("Could not create db handle");
    }

    handle->section = section;
    
    mod_handle = Mod_open(section, "db");
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
DB_init(H handle)
{
    void *mod_handle;
    int (*db_init)(H handle);
    int ret;

    mod_handle = Mod_open(handle->section, "db");
    db_init = (int (*)(H)) Mod_get(mod_handle, "Mod_db_init");
    ret = (*db_init)(handle);
    Mod_close(mod_handle);

    return ret;
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

extern I
DB_get_itr(H handle)
{
    void *mod_handle;
    void (*db_init_itr)(I itr);
    I itr;

    /* Setup the iterator. */
    if((itr = (I) malloc(sizeof(*itr))) == NULL) {
        I_CRIT("Could not create iterator");
    }

    itr->handle  = handle;
    itr->current = 0;
    itr->size    = 0;

    mod_handle = Mod_open(handle->section, "db");
    db_init_itr = (void (*)(I)) Mod_get(mod_handle, "Mod_db_init_itr");
    (*db_init_itr)(itr);
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
        Mod_get(mod_handle, "Mod_itr_next");
    ret = (*db_itr_next)(itr, key, val);
    Mod_close(mod_handle);

    return ret;
}

extern void
DB_close_itr(I *itr)
{
    if(itr || *itr == NULL) {
        return;
    }

    free(*itr);
    *itr = NULL;
}

#undef K
#undef V
#undef H
#undef I
