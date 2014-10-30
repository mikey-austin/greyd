/**
 * @file   bdb.c
 * @brief  Berkeley DB driver.
 * @author Mikey Austin
 * @date   2014
 */

#include "../failures.h"
#include "../greydb.h"

#include <db.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define DEFAULT_PATH "/var/db/greyd"

/*
 * The key/values need to be marshalled before storing, to allow for
 * better on-disk efficiency.
 */
static void pack_key(struct DB_key *key, DBT *dbkey);
static void pack_val(struct DB_val *val, DBT *dbval);
static void unpack_key(struct DB_key *key, DBT *dbkey);
static void unpack_val(struct DB_val *val, DBT *dbval);

extern void
Mod_db_open(DB_handle_T handle, int flags)
{
    char *db_path;
    DB *db;
    int i, ret, open_flags;

    db_path = Config_section_get_str(handle->section, "path", DEFAULT_PATH);

    ret = db_create(&db, NULL, 0);
    if(ret != 0) {
        I_CRIT("Could not obtain db handle: %s", db_strerror(ret));
    }
    handle->dbh = db;

    open_flags = (flags & GREYDB_RO ? DB_RDONLY : DB_CREATE);
    ret = db->open(db, NULL, db_path, NULL, DB_HASH, open_flags, 0600);
    if(ret != 0) {
        switch(ret) {
        case ENOENT:
            /*
             * The database did not exist, so create it.
             */
            i = open(db_path, O_RDWR|O_CREAT, 0644);
            if(i == -1) {
                I_CRIT("Create %s failed (%m)", db_path, strerror(errno));
            }

            /*
             * Set the appropriate owner/group on the new database file.
             */
            if(handle->pw && (fchown(i, handle->pw->pw_uid,
                                     handle->pw->pw_gid) == -1))
            {
                I_CRIT("chown %s failed (%m)", db_path, db_strerror(ret));
            }

            close(i);
            return;

        default:
            I_CRIT("Create %s failed (%m)", db_path, db_strerror(ret));
        }
    }
}

extern void
Mod_db_close(DB_handle_T handle)
{
    DB *db;

    if((db = (DB *) handle->dbh) != NULL) {
        db->close(db, 0);
        handle->dbh = NULL;
    }
}

extern int
Mod_db_put(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    DB *db = (DB *) handle->dbh;
    DBT dbkey, dbval;
    int ret;

    pack_key(key, &dbkey);
    pack_val(val, &dbval);

    ret = db->put(db, NULL, &dbkey, &dbval, 0);

    /* Cleanup the packed data. */
    free(dbkey.data);
    free(dbval.data);

    switch(ret) {
    case 0:
        return GREYDB_OK;

    default:
        I_ERR("Error putting record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern int
Mod_db_get(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    DB *db = (DB *) handle->dbh;
    DBT dbkey, dbval;
    int ret;

    pack_key(key, &dbkey);
    memset(&dbval, 0, sizeof(DBT));

    ret = db->get(db, NULL, &dbkey, &dbval, 0);

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
        I_ERR("Error retrieving record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern int
Mod_db_del(DB_handle_T handle, struct DB_key *key)
{
    DB *db = (DB *) handle->dbh;
    DBT dbkey;
    int ret;

    pack_key(key, &dbkey);
    ret = db->del(db, NULL, &dbkey, 0);

    /* Free packed key data. */
    free(dbkey.data);

    switch(ret) {
    case 0:
        return GREYDB_OK;

    case DB_NOTFOUND:
        return GREYDB_NOT_FOUND;

    default:
        I_ERR("Error deleting record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}

extern void
Mod_db_get_itr(DB_itr_T itr)
{
    DBC *cursor;
    DB *db = (DB *) itr->handle->dbh;
    int ret;

    ret = db->cursor(db, NULL, &cursor, 0);
    if(ret != 0) {
        I_CRIT("Could not create cursor (%s)", db_strerror(ret));
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
        I_WARN("Could not close cursor (%s)", db_strerror(ret));
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
        I_ERR("Error retrieving next record: %s", db_strerror(ret));
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
        I_ERR("Error replacing record: %s", db_strerror(ret));
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
        I_ERR("Error deleting current record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
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
        I_CRIT("Could not pack key");
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
        I_CRIT("Could not pack val");
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
