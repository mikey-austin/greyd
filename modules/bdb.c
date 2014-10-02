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
Mod_db_open(DB_handle_T handle)
{
    char *db_path;
    DB *db;
    int i, ret;

    db_path = Config_section_get_str(handle->section, "path", DEFAULT_PATH);

    ret = db_create(&db, NULL, 0);
    if(ret != 0) {
        I_CRIT("Could not obtain db handle: %s", db_strerror(ret));
    }
    handle->dbh = (void *) db;

    ret = db->open(db, NULL, db_path, NULL, DB_HASH, DB_CREATE, 0600);
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
    dbkey.size = sizeof(struct DB_val);

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
    dbkey.size = sizeof(struct DB_val);

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

extern void
Mod_db_get_itr(DB_itr_T itr)
{
}

extern int
Mod_db_itr_next(DB_itr_T itr, struct DB_key *key, struct DB_val *val)
{
    return 0;
}
