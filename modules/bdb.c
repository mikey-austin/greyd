/**
 * @file   bdb.c
 * @brief  Berkeley DB driver.
 * @author Mikey Austin
 * @date   2014
 */

#include "../greydb.h"
#include <db.h>

extern void
Mod_db_open(DB_handle_T handle)
{
}

extern void
Mod_db_close(DB_handle_T handle)
{
}

extern int
Mod_db_init(DB_handle_T handle)
{
    return 0;
}

extern int
Mod_db_put(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    return 0;
}

extern int
Mod_db_get(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    return 0;
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
