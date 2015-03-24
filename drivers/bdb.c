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
 * @file   bdb.c
 * @brief  Berkeley DB driver.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_DB5_DB_H
#  include <db5/db.h>
#endif
#ifdef HAVE_DB4_DB_H
#  include <db4/db.h>
#endif
#ifdef HAVE_DB_H
#  include <db.h>
#endif

#include "../src/failures.h"
#include "../src/list.h"
#include "../src/greydb.h"

#define DEFAULT_PATH "/var/db/greyd"
#define DEFAULT_DB   "greyd.db"

/**
 * The internal bdb driver handle.
 */
struct bdb_handle {
    DB_ENV *env;
    DB *db;
    DB_TXN *txn;
};

/*
 * The key/values need to be marshalled before storing, to allow for
 * better on-disk efficiency.
 */
static void pack_key(struct DB_key *key, DBT *dbkey);
static void pack_val(struct DB_val *val, DBT *dbval);
static void unpack_key(struct DB_key *key, DBT *dbkey);
static void unpack_val(struct DB_val *val, DBT *dbval);

extern void
Mod_db_init(DB_handle_T handle)
{
    struct bdb_handle *bh;
    char *path;
    int ret, flags, uid_changed = 0;

    path = Config_get_str(handle->config, "path", "database", DEFAULT_PATH);
    if(mkdir(path, 0700) == -1) {
        if(errno != EEXIST)
            i_critical("db environment path: %s", strerror(errno));
    }
    else {
        /*
         * As the directory has just been created, ensure the correct
         * ownership.
         */
        if(handle->pw
           && (chown(path, handle->pw->pw_uid, handle->pw->pw_gid) == -1))
        {
            i_critical("chown %s failed: %s", path, strerror(errno));
        }
    }

    if((bh = malloc(sizeof(*bh))) == NULL)
        i_critical("malloc: %s", strerror(errno));
    bh->env = NULL;
    bh->txn = NULL;
    bh->db = NULL;

    /*
     * We want to create the environment as the database user.
     */
    if(handle->pw) {
        if(seteuid(handle->pw->pw_uid) == -1) {
            i_critical("seteuid: %s", strerror(errno));
        }
        else {
            uid_changed = 1;
        }
    }

    ret = db_env_create(&bh->env, 0);
    if(ret != 0) {
        i_critical("error creating db environment: %s",
                   db_strerror(ret));
    }

    flags = DB_CREATE
        | DB_INIT_TXN
        | DB_INIT_LOCK
        | DB_INIT_LOG
        | DB_INIT_MPOOL;
    ret = bh->env->open(bh->env, path, flags, 0);
    if(ret != 0) {
        i_critical("error opening db environment: %s",
                   db_strerror(ret));
    }

    if(uid_changed && seteuid(0) == -1) {
        bh->env->close(bh->env, 0);
        i_critical("seteuid back to root: %s", strerror(errno));
    }

    handle->dbh = bh;
}

extern void
Mod_db_open(DB_handle_T handle, int flags)
{
    struct bdb_handle *bh = handle->dbh;
    char *db_name, *err_log_path;
    FILE *err_log;
    int ret, open_flags;

    if(bh->db != NULL)
        return;

    db_name = Config_get_str(handle->config, "db_name", "database",
                             DEFAULT_DB);

    ret = db_create(&bh->db, bh->env, 0);
    if(ret != 0) {
        i_warning("Could not obtain db handle: %s", db_strerror(ret));
        goto cleanup;
    }

    open_flags = (flags & GREYDB_RO ? DB_RDONLY : DB_CREATE) | DB_AUTO_COMMIT;
    ret = bh->db->open(bh->db, NULL, db_name, NULL, DB_HASH,
                       open_flags, 0600);
    if(ret != 0) {
        i_warning("db open (%s) failed: %s", db_name, db_strerror(ret));
        goto cleanup;
    }

    if((err_log_path = Config_get_str(handle->config, "error_log", "database", NULL))
       != NULL && (err_log = fopen(err_log_path, "a+")) != NULL)
    {
        bh->db->set_errfile(bh->db, err_log);
    }

    return;

cleanup:
    bh->env->close(bh->env, 0);
    exit(1);
}

extern int
Mod_db_start_txn(DB_handle_T handle)
{
    struct bdb_handle *bh = handle->dbh;
    int ret;

    if(bh->txn != NULL) {
        /* Already in a transaction. */
        return -1;
    }

    ret = bh->env->txn_begin(bh->env, NULL, &bh->txn, 0);
    if(ret != 0) {
        i_warning("db txn start failed: %s", db_strerror(ret));
        goto cleanup;
    }

    return ret;

cleanup:
    if(bh->db)
        bh->db->close(bh->db, 0);
    bh->env->close(bh->env, 0);
    exit(1);
}

extern int
Mod_db_commit_txn(DB_handle_T handle)
{
    struct bdb_handle *bh = handle->dbh;
    int ret;

    if(bh->txn == NULL) {
        i_warning("cannot commit, NULL transaction");
        return -1;
    }

    ret = bh->txn->commit(bh->txn, 0);
    if(ret != 0) {
        i_warning("db txn commit failed: %s", db_strerror(ret));
        goto cleanup;
    }
    bh->txn = NULL;

    return ret;

cleanup:
    if(bh->db)
        bh->db->close(bh->db, 0);
    bh->env->close(bh->env, 0);
    exit(1);
}

extern int
Mod_db_rollback_txn(DB_handle_T handle)
{
    struct bdb_handle *bh = handle->dbh;
    int ret;

    if(bh->txn == NULL) {
        i_warning("cannot rollback, NULL transaction");
        return -1;
    }

    ret = bh->txn->abort(bh->txn);
    bh->txn = NULL;

    return ret;
}

extern void
Mod_db_close(DB_handle_T handle)
{
    struct bdb_handle *bh;

    if((bh = handle->dbh) != NULL) {
        if(bh->db)
            bh->db->close(bh->db, 0);
        bh->env->close(bh->env, 0);
        free(bh);
        handle->dbh = NULL;
    }
}

extern int
Mod_db_put(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    struct bdb_handle *bh = handle->dbh;
    DB *db = bh->db;
    DBT dbkey, dbval;
    int ret;

    pack_key(key, &dbkey);
    pack_val(val, &dbval);

    ret = db->put(db, bh->txn, &dbkey, &dbval, 0);

    /* Cleanup the packed data. */
    free(dbkey.data);
    free(dbval.data);

    switch(ret) {
    case 0:
        return GREYDB_OK;

    default:
        i_error("Error putting record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern int
Mod_db_get(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    struct bdb_handle *bh = handle->dbh;
    DB *db = bh->db;
    DBT dbkey, dbval;
    int ret;

    pack_key(key, &dbkey);
    memset(&dbval, 0, sizeof(DBT));

    ret = db->get(db, bh->txn, &dbkey, &dbval, 0);

    /* Cleanup the packed key data. */
    free(dbkey.data);

    switch(ret) {
    case 0:
        unpack_val(val, &dbval);
        return GREYDB_FOUND;

    case DB_NOTFOUND:
        val = NULL;
        return GREYDB_NOT_FOUND;

    default:
        val = NULL;
        i_error("Error retrieving record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern int
Mod_db_del(DB_handle_T handle, struct DB_key *key)
{
    struct bdb_handle *bh = handle->dbh;
    DB *db = bh->db;
    DBT dbkey;
    int ret;

    pack_key(key, &dbkey);
    ret = db->del(db, bh->txn, &dbkey, 0);

    /* Free packed key data. */
    free(dbkey.data);

    switch(ret) {
    case 0:
        return GREYDB_OK;

    case DB_NOTFOUND:
        return GREYDB_NOT_FOUND;

    default:
        i_error("Error deleting record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern void
Mod_db_get_itr(DB_itr_T itr)
{
    struct bdb_handle *bh = itr->handle->dbh;
    DB *db = bh->db;
    DBC *cursor;
    int ret;

    ret = db->cursor(db, bh->txn, &cursor, 0);
    if(ret != 0) {
        i_critical("Could not create cursor (%s)", db_strerror(ret));
    }
    else {
        itr->dbi = (void *) cursor;
    }
}

extern void
Mod_db_itr_close(DB_itr_T itr)
{
    DBC *cursor = (DBC *) itr->dbi;
    int ret;

    ret = cursor->close(cursor);
    if(ret != 0) {
        i_warning("Could not close cursor (%s)", db_strerror(ret));
    }
    else {
        itr->dbi = NULL;
    }
}

extern int
Mod_db_itr_next(DB_itr_T itr, struct DB_key *key, struct DB_val *val)
{
    DBC *cursor = (DBC *) itr->dbi;
    DBT dbkey, dbval;
    int ret;

    memset(&dbkey, 0, sizeof(DBT));
    memset(&dbval, 0, sizeof(DBT));

    ret = cursor->get(cursor, &dbkey, &dbval, DB_NEXT);
    switch(ret) {
    case 0:
        itr->current++;
        unpack_key(key, &dbkey);
        unpack_val(val, &dbval);
        return GREYDB_FOUND;

    case DB_NOTFOUND:
        return GREYDB_NOT_FOUND;

    default:
        i_error("Error retrieving next record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern int
Mod_db_itr_replace_curr(DB_itr_T itr, struct DB_val *val)
{
    DBC *cursor = (DBC *) itr->dbi;
    DBT dbval;
    int ret;

    pack_val(val, &dbval);
    ret = cursor->put(cursor, NULL, &dbval, DB_CURRENT);

    /* Cleanup packed data. */
    free(dbval.data);

    switch(ret) {
    case 0:
        return GREYDB_OK;

    default:
        i_error("Error replacing record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern int
Mod_db_itr_del_curr(DB_itr_T itr)
{
    DBC *cursor = (DBC *) itr->dbi;
    int ret;

    ret = cursor->del(cursor, 0);
    switch(ret) {
    case 0:
        return GREYDB_OK;

    default:
        i_error("Error deleting current record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern int
Mod_scan_db(DB_handle_T handle, time_t *now, List_T whitelist,
            List_T whitelist_ipv6, List_T traplist, time_t *white_exp)
{
    DB_itr_T itr;
    List_T list;
    struct DB_key key, wkey;
    struct DB_val val, wval;
    struct Grey_tuple gt;
    struct Grey_data gd;
    int ret = 0;

    itr = DB_get_itr(handle);
    while(DB_itr_next(itr, &key, &val) == GREYDB_FOUND) {
        if(val.type != DB_VAL_GREY)
            continue;

        gd = val.data.gd;
        if(gd.expire <= *now && gd.pcount != -2) {
            /*
             * This non-spamtrap entry has expired.
             */
            if(DB_itr_del_curr(itr) != GREYDB_OK) {
                ret = -1;
                goto cleanup;
            }
            else {
                i_debug("deleting expired %sentry %s",
                        (key.type == DB_KEY_IP
                         ? (gd.pcount >= 0 ? "white " : "greytrap ")
                         : "grey "),
                        (key.type == DB_KEY_IP
                         ? key.data.s : key.data.gt.ip));
            }
            continue;
        }
        else if(gd.pcount == -1 && key.type == DB_KEY_IP) {
            /*
             * This is a greytrap hit.
             */
            List_insert_after(traplist, strdup(key.data.s));
        }
        else if(gd.pcount >= 0 && gd.pass <= *now) {
            /*
             * If not already trapped, add the address to the whitelist
             * and add an address-keyed entry to the database.
             */

            if(key.type == DB_KEY_TUPLE) {
                gt = key.data.gt;
                switch(DB_addr_state(handle, gt.ip)) {
                case 1:
                    /* Ignore trapped entries. */
                    continue;

                case -1:
                    ret = -1;
                    goto cleanup;
                }

                list = (IP_check_addr(key.data.s) == AF_INET6
                        ? whitelist_ipv6 : whitelist);
                List_insert_after(list, strdup(key.data.s));

                /* Re-add entry, keyed only by IP address. */
                memset(&wkey, 0, sizeof(wkey));
                wkey.type = DB_KEY_IP;
                wkey.data.s = gt.ip;

                memset(&wval, 0, sizeof(wval));
                wval.type = DB_VAL_GREY;
                gd.expire = *now + *white_exp;
                wval.data.gd = gd;

                if(!(DB_put(handle, &wkey, &wval) == GREYDB_OK
                    && DB_itr_del_curr(itr) == GREYDB_OK))
                {
                    ret = -1;
                    goto cleanup;
                }

                i_debug("whitelisting %s", gt.ip);
            }
            else if(key.type == DB_KEY_IP) {
                /*
                 * This must be a whitelist entry.
                 */
                list = (IP_check_addr(key.data.s) == AF_INET6
                        ? whitelist_ipv6 : whitelist);
                List_insert_after(list, strdup(key.data.s));
            }
        }
    }

cleanup:
    DB_close_itr(&itr);
    return ret;
}

static void
pack_key(struct DB_key *key, DBT *dbkey)
{
    char *buf = NULL, *s;
    int len, slen = 0;

    len = sizeof(short) + (key->type == DB_KEY_TUPLE
                           ? (slen = (strlen(key->data.gt.ip) + 1
                                      + strlen(key->data.gt.helo) + 1
                                      + strlen(key->data.gt.from) + 1
                                      + strlen(key->data.gt.to) + 1))
                           : (slen = strlen(key->data.s) + 1));
    if((buf = calloc(sizeof(char), len)) == NULL) {
        i_critical("Could not pack key");
    }
    memcpy(buf, &(key->type), sizeof(short));

    switch(key->type) {
    case DB_KEY_IP:
    case DB_KEY_MAIL:
        memcpy(buf + sizeof(short), key->data.s, slen);
        break;

    case DB_KEY_TUPLE:
        s = buf + sizeof(short);
        memcpy(s, key->data.gt.ip, (slen = (strlen(key->data.gt.ip) + 1)));
        s += slen;

        memcpy(s, key->data.gt.helo,
               (slen = (strlen(key->data.gt.helo) + 1)));
        s += slen;

        memcpy(s, key->data.gt.from,
               (slen = (strlen(key->data.gt.from) + 1)));
        s += slen;

        memcpy(s, key->data.gt.to, (slen = (strlen(key->data.gt.to) + 1)));
        break;
    }

    memset(dbkey, 0, sizeof(*dbkey));
    dbkey->data = buf;
    dbkey->size = len;
}

static void
pack_val(struct DB_val *val, DBT *dbval)
{
    char *buf = NULL;
    int len, slen = 0;

    len = sizeof(short) + (val->type == DB_VAL_GREY
                           ? sizeof(struct Grey_data)
                           : (slen = strlen(val->data.s) + 1));
    if((buf = calloc(sizeof(char), len)) == NULL) {
        i_critical("Could not pack val");
    }
    memcpy(buf, &(val->type), sizeof(short));

    switch(val->type) {
    case DB_VAL_MATCH_SUFFIX:
        memcpy(buf + sizeof(short), val->data.s, slen);
        break;

    case DB_VAL_GREY:
        memcpy(buf + sizeof(short), &(val->data.gd),
               sizeof(struct Grey_data));
        break;
    }

    memset(dbval, 0, sizeof(*dbval));
    dbval->data = buf;
    dbval->size = len;
}

static void
unpack_key(struct DB_key *key, DBT *dbkey)
{
    char *buf = (char *) dbkey->data;

    memset(key, 0, sizeof(*key));
    key->type = *((short *) buf);
    buf += sizeof(short);

    switch(key->type) {
    case DB_KEY_IP:
    case DB_KEY_MAIL:
        key->data.s = buf;
        break;

    case DB_KEY_TUPLE:
        key->data.gt.ip = buf;
        buf += strlen(key->data.gt.ip) + 1;

        key->data.gt.helo = buf;
        buf += strlen(key->data.gt.helo) + 1;

        key->data.gt.from = buf;
        buf += strlen(key->data.gt.from) + 1;

        key->data.gt.to = buf;
        break;
    }
}

static void
unpack_val(struct DB_val *val, DBT *dbval)
{
    char *buf = (char *) dbval->data;

    memset(val, 0, sizeof(*val));
    val->type = *((short *) buf);
    buf += sizeof(short);

    switch(val->type) {
    case DB_VAL_MATCH_SUFFIX:
        val->data.s = buf;
        break;

    case DB_VAL_GREY:
        val->data.gd = *((struct Grey_data *) buf);
        break;
    }
}
