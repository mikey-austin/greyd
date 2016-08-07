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
DB_init(Config_T config)
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
    if(Config_get_int(config, "drop_privs", NULL, 1)
       && (user = Config_get_str(config, "user", "grey", GREYD_DB_USER)) != NULL)
    {
        if((handle->pw = getpwnam(user)) == NULL) {
            i_critical("No such user %s", user);
        }
    }
    else {
        handle->pw = NULL;
    }

    /* Open the configured driver and extract all required symbols. */
    handle->driver = Mod_open(section);

    handle->db_init = (void (*)(DB_handle_T))
        Mod_get(handle->driver, "Mod_db_init");
    handle->db_open = (void (*)(DB_handle_T, int))
        Mod_get(handle->driver, "Mod_db_open");
    handle->db_start_txn = (int (*)(DB_handle_T))
        Mod_get(handle->driver, "Mod_db_start_txn");
    handle->db_commit_txn = (int (*)(DB_handle_T))
        Mod_get(handle->driver, "Mod_db_commit_txn");
    handle->db_rollback_txn = (int (*)(DB_handle_T))
        Mod_get(handle->driver, "Mod_db_rollback_txn");
    handle->db_close = (void (*)(DB_handle_T))
        Mod_get(handle->driver, "Mod_db_close");
    handle->db_put = (int (*)(DB_handle_T, struct DB_key *, struct DB_val *))
        Mod_get(handle->driver, "Mod_db_put");
    handle->db_get = (int (*)(DB_handle_T, struct DB_key *, struct DB_val *))
        Mod_get(handle->driver, "Mod_db_get");
    handle->db_del = (int (*)(DB_handle_T, struct DB_key *))
        Mod_get(handle->driver, "Mod_db_del");
    handle->db_get_itr = (void (*)(DB_itr_T, int))
        Mod_get(handle->driver, "Mod_db_get_itr");
    handle->db_itr_next = (int (*)(DB_itr_T, struct DB_key *, struct DB_val *))
        Mod_get(handle->driver, "Mod_db_itr_next");
    handle->db_itr_replace_curr = (int (*)(DB_itr_T, struct DB_val *))
        Mod_get(handle->driver, "Mod_db_itr_replace_curr");
    handle->db_itr_del_curr = (int (*)(DB_itr_T))
        Mod_get(handle->driver, "Mod_db_itr_del_curr");
    handle->db_itr_close = (void (*)(DB_itr_T))
        Mod_get(handle->driver, "Mod_db_itr_close");
    handle->db_scan = (int (*)(DB_handle_T, time_t *, List_T, List_T,
                               List_T, time_t *))
        Mod_get(handle->driver, "Mod_scan_db");

    /* Initialize the database driver. */
    handle->db_init(handle);

    return handle;
}

extern void
DB_open(DB_handle_T handle, int flags)
{
    handle->db_open(handle, flags);
}

extern int
DB_start_txn(DB_handle_T handle)
{
    return handle->db_start_txn(handle);
}

extern int
DB_commit_txn(DB_handle_T handle)
{
    return handle->db_commit_txn(handle);
}

extern int
DB_rollback_txn(DB_handle_T handle)
{
    return handle->db_rollback_txn(handle);
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
DB_get_itr(DB_handle_T handle, int types)
{
    DB_itr_T itr;

    /* Setup the iterator. */
    if((itr = malloc(sizeof(*itr))) == NULL) {
        i_critical("Could not create iterator");
    }

    itr->handle  = handle;
    itr->current = -1;
    itr->size    = 0;

    handle->db_get_itr(itr, types);

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

extern int
DB_scan(DB_handle_T handle, time_t *now, List_T whitelist,
        List_T whitelist_ipv6, List_T traplist, time_t *white_exp)
{
    return handle->db_scan(handle, now, whitelist, whitelist_ipv6, traplist,
                           white_exp);
}

extern int
DB_addr_state(DB_handle_T handle, char *addr)
{
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;
    int ret;

    key.type = DB_KEY_IP;
    key.data.s = addr;
    ret = DB_get(handle, &key, &val);

    switch(ret) {
    case GREYDB_NOT_FOUND:
        return 0;

    case GREYDB_FOUND:
        gd = val.data.gd;
        return (gd.pcount == -1 ? 1 : 2);

    default:
        return -1;
    }
}
