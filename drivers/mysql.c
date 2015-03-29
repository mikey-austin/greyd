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
 * @file   mysql.c
 * @brief  MySQL DB driver.
 * @author Mikey Austin
 * @date   2015
 */

#include <config.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_MYSQL_H
#  include <mysql.h>
#endif
#ifdef HAVE_MY_GLOBAL_H
#  include <my_global.h>
#endif

#include "../src/failures.h"
#include "../src/greydb.h"
#include "../src/utils.h"

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 3306
#define DEFAULT_DB   "greyd"

/**
 * The internal driver handle.
 */
struct mysql_handle {
    MYSQL *db;
    char *greyd_host;
    int txn;
    int connected;
};

struct s3_itr {
    MYSQL_RES *res;
    struct DB_key *curr;
};

static void populate_key(MYSQL_ROW, struct DB_key *, int);
static void populate_val(MYSQL_ROW, struct DB_val *, int);

extern void
Mod_db_init(DB_handle_T handle)
{
    struct mysql_handle *dbh;
    char *path;
    int ret, flags, uid_changed = 0, len;

    if((dbh = malloc(sizeof(*dbh))) == NULL)
        i_critical("malloc: %s", strerror(errno));

    handle->dbh = dbh;
    dbh->db = mysql_init(NULL);
    dbh->txn = 0;
    dbh->connected = 0;

    /* Escape and store the hostname in a static buffer. */
    dbh->greyd_host = Config_get_str(handle->config, "hostname", NULL, '');
    static char host_esc[2 * (len = strlen(dbh->greyd_host)) + 1];
    mysql_real_escape(dbh->db, host_esc, dbh->greyd_host, len);
    dbh->greyd_host = host_esc;
}

extern void
Mod_db_open(DB_handle_T handle, int flags)
{
    struct mysql_handle *dbh = handle->dbh;
    char *name, *host, *user, *pass, *socket, *sql;
    int port;

    if(dbh->connected)
        return;

    host = Config_get_str(handle->config, "host", "database", DEFAULT_HOST);
    port = Config_get_int(handle->config, "port", "database", DEFAULT_PORT)
    name = Config_get_str(handle->config, "name", "database", DEFAULT_DB);
    socket = Config_get_str(handle->config, "socket", "database", NULL);
    user = Config_get_str(handle->config, "user", "database", NULL);
    pass = Config_get_str(handle->config, "pass", "database", NULL);

    if(mysql_real_connect(dbh->db, host, user, pass,
                          name, port, socket, 0) == NULL)
    {
        i_warning("could not connect to mysql %s:%d: %s",
                  host, port,
                  mysql_error(dbh->db));
        goto cleanup;
    }
    dbh->connected = 1;

    return;

cleanup:
    exit(1);
}

extern int
Mod_db_start_txn(DB_handle_T handle)
{
    struct mysql_handle *dbh = handle->dbh;
    int ret;

    if(dbh->txn != 0) {
        /* Already in a transaction. */
        return -1;
    }

    ret = mysql_query(dbh->db, "START TRANSACTION");
    if(ret != 0) {
        i_warning("start txn failed: %s", mysql_error(dbh->db));
        goto cleanup;
    }
    dbh->txn = 1;

    return ret;

cleanup:
    if(dbh->db)
        mysql_close(dbh->db);
    exit(1);
}

extern int
Mod_db_commit_txn(DB_handle_T handle)
{
    struct mysql_handle *dbh = handle->dbh;

    if(dbh->txn != 1) {
        i_warning("cannot commit, not in transaction");
        return -1;
    }

    if(!mysql_commit(dbh->db)) {
        i_warning("db txn commit failed: %s", mysql_error(dbh->db));
        goto cleanup;
    }
    dbh->txn = 0;
    return 0;

cleanup:
    if(dbh->db)
        mysql_close(dbh->db);
    exit(1);
}

extern int
Mod_db_rollback_txn(DB_handle_T handle)
{
    struct mysql_handle *dbh = handle->dbh;

    if(dbh->txn != 1) {
        i_warning("cannot rollback, not in transaction");
        return -1;
    }

    if(!mysql_rollback(dbh->db)) {
        i_warning("db txn rollback failed: %s", mysql_error(dbh->db));
        goto cleanup;
    }
    dbh->txn = 0;
    return 0;

cleanup:
    if(dbh->db)
        mysql_close(dbh->db);
    exit(1);
}

extern void
Mod_db_close(DB_handle_T handle)
{
    struct s3_handle *dbh;

    if((dbh = handle->dbh) != NULL) {
        if(dbh->db)
            mysql_close(dbh->db);
        free(dbh);
        handle->dbh = NULL;
    }
}

extern int
Mod_db_put(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    struct mysql_handle *dbh = handle->dbh;
    char *sql, *sql_tmpl;
    struct Grey_tuple *gt;
    struct Grey_data *gd;
    unsigned long len;

    switch(key->type) {
    case DB_KEY_MAIL:
        sql_tmpl = "INSERT IGNORE INTO spamtraps(address) VALUES ('%s')";

        char add_esc[2 * (len = strlen(key->data.s)) + 1];
        mysql_real_escape_string(dbh->db, add_esc, key->data.s, len);

        if(asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    case DB_KEY_IP:
        sql_tmpl = "REPLACE INTO entries "
            "(`ip`, `helo`, `from`, `to`, "
            " `first`, `pass`, `expire`, `bcount`, `pcount`, `greyd_host`) "
            "VALUES "
            "('%s', '', '', '', %lld, %lld, %lld, %ld, %ld, '%s')";

        char ip_esc[2 * (len = strlen(key->data.s)) + 1];
        mysql_real_escape_string(dbh->db, ip_esc, key->data.s, len);

        gd = &val->data.gd;
        if(asprintf(&sql, sql_tmpl, ip_esc, gd->first, gd->pass,
                    gd->expire, gd->bcount, gd->pcount,
                    dbh->greyd_host) == -1)
        {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    case DB_KEY_TUPLE:
        sql_tmpl = "REPLACE INTO entries "
            "(`ip`, `helo`, `from`, `to`, "
            " `first`, `pass`, `expire`, `bcount`, `pcount`, `greyd_host`) "
            "VALUES "
            "('%s', '%s', '%s', '%s', %lld, %lld, %lld, %ld, %ld, '%s')";

        gt = &key->data.gt;
        char ip_esc[2 * (len = strlen(gt->ip)) + 1];
        mysql_real_escape_string(dbh->db, ip_esc, gt->ip, len);

        char helo_esc[2 * (len = strlen(gt->helo)) + 1];
        mysql_real_escape_string(dbh->db, helo_esc, gt->helo, len);

        char from_esc[2 * (len = strlen(gt->from)) + 1];
        mysql_real_escape_string(dbh->db, from_esc, gt->from, len);

        char to_esc[2 * (len = strlen(gt->to)) + 1];
        mysql_real_escape_string(dbh->db, to_esc, gt->to, len);

        gd = &val->data.gd;
        if(asprintf(&sql, sql_tmpl, ip_esc, helo_esc, from_esc, to_esc,
                    gd->first, gd->pass, gd->expire, gd->bcount, gd->pcount,
                    dbh->greyd_host) == -1)
        {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    default:
        return GREYDB_ERR;
    }

    if(mysql_real_query(dbh->db, sql, strlen(sql)) != 0) {
        i_warning("insert mysql error: %s", mysql_error(dbh->db));
        free(sql);
        goto err;
    }
    free(sql);
    return GREYDB_OK;

err:
    DB_rollback_txn(handle);
    return GREYDB_ERR;
}

extern int
Mod_db_get(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    struct mysql_handle *dbh = handle->dbh;
    char *sql = NULL, *sql_tmpl;
    struct Grey_tuple *gt;
    unsigned long len;
    int res = GREYDB_NOT_FOUND;

    switch(key->type) {
    case DB_KEY_MAIL:
        sql = "SELECT 0, 0, 0, 0, -2 "
            "FROM spamtraps WHERE `address`='%s' "
            "LIMIT 1";

        char add_esc[2 * (len = strlen(key->data.s)) + 1];
        mysql_real_escape_string(dbh->db, add_esc, key->data.s, len);

        if(asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    case DB_KEY_IP:
        sql = "SELECT `first`, `pass`, `expire`, `bcount`, `pcount`"
            "FROM entries "
            "WHERE `ip`='%s' AND `helo`='' AND `from`='' AND `to`='' "
            "LIMIT 1";

        char add_esc[2 * (len = strlen(key->data.s)) + 1];
        mysql_real_escape_string(dbh->db, add_esc, key->data.s, len);

        if(asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    case DB_KEY_TUPLE:
        sql = "SELECT `first`, `pass`, `expire`, `bcount`, `pcount`"
            "FROM entries "
            "WHERE `ip`='%s' AND `helo`='%s' AND `from`='%s' AND `to`='%s' "
            "LIMIT 1";

        gt = &key->data.gt;
        char ip_esc[2 * (len = strlen(gt->ip)) + 1];
        mysql_real_escape_string(dbh->db, ip_esc, gt->ip, len);

        char helo_esc[2 * (len = strlen(gt->helo)) + 1];
        mysql_real_escape_string(dbh->db, helo_esc, gt->helo, len);

        char from_esc[2 * (len = strlen(gt->from)) + 1];
        mysql_real_escape_string(dbh->db, from_esc, gt->from, len);

        char to_esc[2 * (len = strlen(gt->to)) + 1];
        mysql_real_escape_string(dbh->db, to_esc, gt->to, len);

        gd = &val->data.gd;
        if(asprintf(&sql, sql_tmpl, ip_esc, helo_esc, from_esc,
                    to_esc) == -1)
        {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    default:
        return GREYDB_ERR;
    }

    MYSQL_RES *result = NULL;
    MYSQL_ROW row;
    MYSQL_FIELD *field;
    unsigned int expected_fields = 5;

    if(mysql_real_query(dbh->db, sql, strlen(sql)) != 0) {
        i_warning("insert mysql error: %s", mysql_error(dbh->db));
        goto err;
    }

    if((result = mysql_store_result(dbh->db)) != NULL
       && mysql_num_fields(result) == expected_fields
       && mysql_num_rows(result) == 1)
    {
        res = GREYDB_FOUND;
        row = mysql_fetch_row(result);
        populate_val(row, val, 0);
    }

err:
    if(sql != NULL)
        free(sql);
    if(result != NULL)
        mysql_free_result(result);
    return res;
}

extern int
Mod_db_del(DB_handle_T handle, struct DB_key *key)
{
    struct mysql_handle *dbh = handle->dbh;
    char *sql, *sql_tmpl;
    struct Grey_data *gd;
    unsigned long len;

    switch(key->type) {
    case DB_KEY_MAIL:
        sql_tmpl = "DELETE FROM spamtraps WHERE `address`='%s'";

        char add_esc[2 * (len = strlen(key->data.s)) + 1];
        mysql_real_escape_string(dbh->db, add_esc, key->data.s, len);

        if(asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    case DB_KEY_IP:
        sql = "DELETE FROM entries WHERE `ip`='%s' "
            "AND `helo`='' AND `from`='' AND `to`=''"
            "AND `greyd_host`='%s'";

        char add_esc[2 * (len = strlen(key->data.s)) + 1];
        mysql_real_escape_string(dbh->db, add_esc, key->data.s, len);

        if(asprintf(&sql, sql_tmpl, add_esc, dbh->greyd_host) == -1) {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    case DB_KEY_TUPLE:
        sql = "DELETE FROM entries WHERE `ip`='%s' "
            "AND `helo`='%s' AND `from`='%s' AND `to`='%s' "
            "AND `greyd_host`='%s'";

        gt = &key->data.gt;
        char ip_esc[2 * (len = strlen(gt->ip)) + 1];
        mysql_real_escape_string(dbh->db, ip_esc, gt->ip, len);

        char helo_esc[2 * (len = strlen(gt->helo)) + 1];
        mysql_real_escape_string(dbh->db, helo_esc, gt->helo, len);

        char from_esc[2 * (len = strlen(gt->from)) + 1];
        mysql_real_escape_string(dbh->db, from_esc, gt->from, len);

        char to_esc[2 * (len = strlen(gt->to)) + 1];
        mysql_real_escape_string(dbh->db, to_esc, gt->to, len);

        gd = &val->data.gd;
        if(asprintf(&sql, sql_tmpl, ip_esc, helo_esc, from_esc, to_esc,
                    dbh->greyd_host) == -1)
        {
            i_warning("mysql asprintf error");
            goto err;
        }
        break;

    default:
        return GREYDB_ERR;
    }

    if(mysql_real_query(dbh->db, sql, strlen(sql)) != 0) {
        i_warning("insert mysql error: %s", mysql_error(dbh->db));
        free(sql);
        goto err;
    }
    free(sql);
    return GREYDB_OK;

err:
    DB_rollback_txn(handle);
    return GREYDB_ERR;
}

extern void
Mod_db_get_itr(DB_itr_T itr)
{
    struct mysql_handle *dbh = itr->handle->dbh;
    struct mysql_itr *dbi = NULL;
    char *sql;

    if((dbi = malloc(sizeof(*dbi))) == NULL) {
        i_warning("malloc: %s", strerror(errno));
        goto err;
    }
    dbi->res = NULL;
    dbi->curr = NULL;
    itr->dbi = dbi;

    sql = "SELECT `ip`, `helo`, `from`, `to`, "
        "`first`, `pass`, `expire`, `bcount`, `pcount` FROM entries "
        "UNION "
        "SELECT `address`, '', '', '', 0, 0, 0, 0, -2 FROM spamtraps";

    if(mysql_real_query(dbh->db, sql, strlen(sql)) != 0) {
        i_warning("insert mysql error: %s", mysql_error(dbh->db));
        free(sql);
        goto err;
    }

    dbi->res = mysql_store_result(dbh->db);
    itr->size = mysql_num_rows(dbi->res);

    return;

err:
    DB_rollback_txn(itr->handle);
    exit(1);
}

extern void
Mod_db_itr_close(DB_itr_T itr)
{
    struct mysql_handle *dbh = itr->handle->dbh;
    struct mysql_itr *dbi = itr->dbi;

    if(dbi) {
        dbi->curr = NULL;
        if(dbi->res != NULL)
            mysql_free_result(dbi->res);
        free(dbi);
        itr->dbi = NULL;
    }
}

extern int
Mod_db_itr_next(DB_itr_T itr, struct DB_key *key, struct DB_val *val)
{
    struct mysql_itr *dbi = itr->dbi;
    MYSQL_ROW row;

    if(dbi->res == NULL && itr->current < itr->size)
        return GREYDB_NOT_FOUND;

    row = sqlite3_step(dbi->res);
    populate_key(row, key, 0);
    populate_val(row, val, 4);
    dbi->curr = key;
    itr->current++;

    return GREYDB_FOUND;
}

extern int
Mod_db_itr_replace_curr(DB_itr_T itr, struct DB_val *val)
{
    struct mysql_handle *dbh = itr->handle->dbh;
    struct mysql_itr *dbi = itr->dbi;
    struct DB_key *key = dbi->curr;

    return DB_put(itr->handle, key, val);
}

extern int
Mod_db_itr_del_curr(DB_itr_T itr)
{
    struct mysql_itr *dbi = itr->dbi;

    if(dbi && dbi->stmt && dbi->curr)
        return DB_del(itr->handle, dbi->curr);

    return GREYDB_ERR;
}

extern int
Mod_scan_db(DB_handle_T handle, time_t *now, List_T whitelist,
            List_T whitelist_ipv6, List_T traplist, time_t *white_exp)
{
    struct mysql_handle *dbh = handle->dbh;
    sqlite3_stmt *stmt;
    char *sql;
    int ret = GREYDB_OK;

    /*
     * Delete expired entries and whitelist appropriate grey entries,
     * by un-setting the tuple fields (to, from, helo), but only if there
     * is not already a conflicting entry with the same IP address
     * (ie an existing trap entry).
     */
    sql = "DELETE FROM entries WHERE expire <= ?";

    if(!(!sqlite3_prepare(dbh->db, sql, strlen(sql) + 1, &stmt, NULL)
         && !sqlite3_bind_int64(stmt, 1, *now)))
    {
        i_warning("delete expired entries: %s", sqlite3_errmsg(dbh->db));
        ret = GREYDB_ERR;
        goto cleanup;
    }

    if(sqlite3_step(stmt) != SQLITE_DONE) {
        i_warning("sqlite3_step: %s", sqlite3_errmsg(dbh->db));
        ret = GREYDB_ERR;
        goto cleanup;
    }
    sqlite3_finalize(stmt);

    sql = "UPDATE entries "
        "SET `helo` = '', `from` = '', `to` = '', `expire` = ? "
        "WHERE `from` <> '' AND `to` <> '' AND `pcount` >= 0 AND `pass` <= ? "
        "AND `ip` NOT IN ( "
        "    SELECT `ip` FROM entries WHERE `from` = '' AND `to` = '' "
        ")";

    if(!(!sqlite3_prepare(dbh->db, sql, strlen(sql) + 1, &stmt, NULL)
         && !sqlite3_bind_int64(stmt, 1, *now + *white_exp)
         && !sqlite3_bind_int64(stmt, 2, *now)))
    {
        i_warning("update db entries: %s", sqlite3_errmsg(dbh->db));
        ret = GREYDB_ERR;
        goto cleanup;
    }

    if(sqlite3_step(stmt) != SQLITE_DONE) {
        i_warning("sqlite3_step: %s", sqlite3_errmsg(dbh->db));
        ret = GREYDB_ERR;
        goto cleanup;
    }
    sqlite3_finalize(stmt);

    /* Add greytrap & whitelist entries. */
    sql = "SELECT `ip`, NULL, NULL FROM entries             \
        WHERE `to`='' AND `from`='' AND `ip` NOT LIKE '%:%' \
            AND `pcount` >= 0                               \
        UNION                                               \
        SELECT NULL, `ip`, NULL FROM entries                \
        WHERE `to`='' AND `from`='' AND `ip` LIKE '%:%'     \
            AND `pcount` >= 0                               \
        UNION                                               \
        SELECT NULL, NULL, `ip` FROM entries                \
        WHERE `to`='' AND `from`='' AND `pcount` < 0";

    if(sqlite3_prepare(dbh->db, sql, strlen(sql) + 1, &stmt, NULL)) {
        i_warning("fetch white/trap entries: %s", sqlite3_errmsg(dbh->db));
        ret = GREYDB_ERR;
        goto cleanup;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW) {
        if(sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            List_insert_after(
                whitelist,
                strdup((const char *) sqlite3_column_text(stmt, 0)));
        }
        else if(sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            List_insert_after(
                whitelist_ipv6,
                strdup((const char *) sqlite3_column_text(stmt, 1)));
        }
        else if(sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
            List_insert_after(
                traplist,
                strdup((const char *) sqlite3_column_text(stmt, 2)));
        }
    }

cleanup:
    sqlite3_finalize(stmt);
    return ret;
}

static void
populate_key(MYSQL_ROW row, struct DB_key *key, int from)
{
    struct Grey_tuple *gt;
    static char buf[(INET6_ADDRSTRLEN + 1) + 3 * (GREY_MAX_MAIL + 1)];
    char *buf_p = buf;

    memset(key, 0, sizeof(*key));
    memset(buf, 0, sizeof(buf));

    /*
     * Empty helo, from & to columns indicate a non-grey entry.
     * A pcount of -2 indicates a spamtrap, which has a key type
     * of DB_KEY_MAIL.
     */
    key->type = ((!memcmp(row[1], "", 1)
                  && !memcmp(row[2], "", 1)
                  && !memcmp(row[3], "", 1))
                 ? (atoi(row[8]) == -2
                    ? DB_KEY_MAIL : DB_KEY_IP)
                 : DB_KEY_TUPLE);

    if(key->type == DB_KEY_IP) {
        key->data.s = buf_p;
        sstrncpy(buf_p, (const char *) row[from + 0]), INET6_ADDRSTRLEN + 1);
    }
    else if(key->type == DB_KEY_MAIL) {
        key->data.s = buf_p;
        sstrncpy(buf_p, (const char *) row[from + 0], GREY_MAX_MAIL + 1);
    }
    else {
        gt = &key->data.gt;
        gt->ip = buf_p;
        sstrncpy(buf_p, (const char *) row[from + 0], INET6_ADDRSTRLEN + 1);
        buf_p += INET6_ADDRSTRLEN + 1;

        gt->helo = buf_p;
        sstrncpy(buf_p, (const char *) row[from + 1], GREY_MAX_MAIL + 1);
        buf_p += GREY_MAX_MAIL + 1;

        gt->from = buf_p;
        sstrncpy(buf_p, (const char *) row[from + 2], GREY_MAX_MAIL + 1);
        buf_p += GREY_MAX_MAIL + 1;

        gt->to = buf_p;
        sstrncpy(buf_p, (const char *) row[from + 3], GREY_MAX_MAIL + 1);
        buf_p += GREY_MAX_MAIL + 1;
    }
}

static void
populate_val(MYSQL_ROW row, struct DB_val *val, int from)
{
    struct Grey_data *gd;

    memset(val, 0, sizeof(*val));
    val->type = DB_VAL_GREY;
    gd = &val->data.gd;
    gd->first = row[from + 0];
    gd->pass = row[from + 1];
    gd->expire = row[from + 2];
    gd->bcount = row[from + 3];
    gd->pcount = row[from + 4];
}
