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
    handle->dbh = (void *) db;

    open_flags = DB_CREATE | (flags & GREYDB_RO ? DB_RDONLY : 0);
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
    DBT dbkey, data;
    int ret;

    memset(&dbkey, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    dbkey.data = key;
    dbkey.size = sizeof(struct DB_key);

    data.data = val;
    data.size = sizeof(struct DB_val);

    ret = db->put(db, NULL, &dbkey, &data, 0);
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
    DBT dbkey, data;
    int ret;

    memset(&dbkey, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    memset(val, 0, sizeof(struct DB_val));

    dbkey.data = key;
    dbkey.size = sizeof(struct DB_key);

    data.data = val;
    data.ulen = sizeof(struct DB_val);
    data.flags = DB_DBT_USERMEM;

    ret = db->get(db, NULL, &dbkey, &data, 0);
    switch(ret) {
    case 0:
        return GREYDB_FOUND;

    case DB_NOTFOUND:
        return GREYDB_NOT_FOUND;

    default:
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

    memset(&dbkey, 0, sizeof(DBT));
    dbkey.data = key;
    dbkey.size = sizeof(struct DB_key);

    ret = db->del(db, NULL, &dbkey, 0);
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
    DBT dbkey, data;
    int ret;

    memset(&dbkey, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
    memset(key, 0, sizeof(struct DB_key));
    memset(val, 0, sizeof(struct DB_val));

    dbkey.data = key;
    dbkey.ulen = sizeof(struct DB_key);
    dbkey.flags = DB_DBT_USERMEM;

    data.data = val;
    data.ulen = sizeof(struct DB_val);
    data.flags = DB_DBT_USERMEM;

    ret = cursor->get(cursor, &dbkey, &data, DB_NEXT);
    switch(ret) {
    case 0:
        itr->current++;
        return GREYDB_FOUND;

    case DB_NOTFOUND:
        return GREYDB_NOT_FOUND;

    default:
        I_ERR("Error retrieving next record: %s", db_strerror(ret));
    }

    return GREYDB_ERR;
}
