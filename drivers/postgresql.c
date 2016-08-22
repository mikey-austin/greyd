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
 * @file   postgresql.c
 * @brief  PostgreSQL DB driver.
 * @author Mikey Austin
 * @date   2016
 */

#include <config.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_LIBPQ_FE_H
#    include <libpq-fe.h>
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
struct postgresql_handle {
    PGconn *db;
    char *greyd_host;
    int txn;
    int connected;
};

struct mysql_itr {
    PGresult *res;
    struct DB_key *curr;
};

static char *escape(PGconn *, const char *);
static void populate_key(PGresult *, struct DB_key *, int);
static void populate_val(PGresult *, struct DB_val *, int);

extern void
Mod_db_init(DB_handle_T handle)
{
    struct postgresql_handle *dbh;
    char *path, *hostname;
    int ret, flags, uid_changed = 0, len;
    int *error;

    if((dbh = malloc(sizeof(*dbh))) == NULL)
        i_critical("malloc: %s", strerror(errno));

    handle->dbh = dbh;
    dbh->db = PQconnectdb(""); // TODO cleaner?
    dbh->txn = 0;
    dbh->connected = 0;

    /* Escape and store the hostname in a static buffer. */
    hostname = Config_get_str(handle->config, "hostname", NULL, "");
    if((dbh->greyd_host = calloc(2 * (len = strlen(hostname)) + 1,
                                 sizeof(char))) == NULL)
    {
        i_critical("malloc: %s", strerror(errno));
    }
    PQescapeStringConn(dbh->db, dbh->greyd_host, hostname, len, error);
}

extern void
Mod_db_open(DB_handle_T handle, int flags)
{
    struct postgresql_handle *dbh = handle->dbh;
    char *dbname, *host, *user, *password, *socket, *sql;
    char port[4];
    int expand_dbname = 0;

    if(dbh->connected)
        return;

    host = Config_get_str(handle->config, "host", "database", DEFAULT_HOST);
    dbname = Config_get_str(handle->config, "name", "database", DEFAULT_DB);
    socket = Config_get_str(handle->config, "socket", "database", NULL);
    user = Config_get_str(handle->config, "user", "database", NULL);
    password = Config_get_str(handle->config, "pass", "database", NULL);

    snprintf(port, 4, "%d", Config_get_int(handle->config, "port", "database",
                                           DEFAULT_PORT));

    // TODO cleaner? host - socket.
    const char *db_keywords[] = {"host", "port", "dbname", "socket", "user",
                                 "password", NULL};
    const char *db_values[] = {host, port, dbname, socket, user, password,
                               NULL};

    dbh->db = PQconnectdbParams(db_keywords, db_values, expand_dbname);
    if(dbh->db == NULL)
    {
        i_warning("could not connect to postgresql %s:%d: %s", host, port,
                  PQerrorMessage(dbh->db));
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
    struct postgresql_handle *dbh = handle->dbh;
    PGresult *res;

    if(dbh->txn != 0) {
        /* Already in a transaction. */
        return -1;
    }

    res = PQexec(dbh->db, "BEGIN");
    if(PQresultStatus(res) != PGRES_COMMAND_OK) {
        i_warning("start txn failed: %s", PQerrorMessage(dbh->db));
        goto cleanup;
    }
    dbh->txn = 1;

    return 1;

cleanup:
    if(dbh->db) {
      PQclear(res);
      PQfinish(dbh->db);
    }
    exit(1);
}

extern int
Mod_db_commit_txn(DB_handle_T handle)
{
    struct postgresql_handle *dbh = handle->dbh;
    PGresult *res;

    if(dbh->txn != 1) {
        i_warning("cannot commit, not in transaction");
        return -1;
    }

    res = PQexec(dbh->db, "END");
    if(PQresultStatus(res) != PGRES_COMMAND_OK) {
        i_warning("db txn commit failed: %s", PQerrorMessage(dbh->db));
        goto cleanup;
    }
    dbh->txn = 0;
    PQclear(res);

    return 0;

cleanup:
    if(dbh->db) {
      PQclear(res);
      PQfinish(dbh->db);
    }
    exit(1);
}

extern int
Mod_db_rollback_txn(DB_handle_T handle)
{
    struct postgresql_handle *dbh = handle->dbh;
    PGresult *res;

    if(dbh->txn != 1) {
        i_warning("cannot rollback, not in transaction");
        return -1;
    }

    res = PQexec(dbh->db, "ROLLBACK");
    if(PQresultStatus(res) != PGRES_COMMAND_OK) {
        i_warning("db txn rollback failed: %s", PQerrorMessage(dbh->db));
        goto cleanup;
    }
    dbh->txn = 0;
    return 0;

cleanup:
    if(dbh->db) {
      PQclear(res);
      PQfinish(dbh->db);
    }
    exit(1);
}

extern void
Mod_db_close(DB_handle_T handle)
{
    struct postgresql_handle *dbh = handle->dbh;

    if((dbh = handle->dbh) != NULL) {
        free(dbh->greyd_host);
        if(dbh->db)
            PQfinish(dbh->db);
        free(dbh);
        handle->dbh = NULL;
    }
}

extern int
Mod_db_put(DB_handle_T handle, struct DB_key *key, struct DB_val *val)
{
    struct postgreql_handle *dbh = handle->dbh;
    char *sql = NULL, *sql_tmpl = NULL;
    struct Grey_tuple *gt;
    struct Grey_data *gd;
    unsigned long len;
    char *add_esc = NULL;
    char *ip_esc = NULL;
    char *helo_esc = NULL;
    char *from_esc = NULL;
    char *to_esc = NULL;

    switch(key->type) {
    case DB_KEY_MAIL:
        sql_tmpl = "INSERT IGNORE INTO spamtraps(address) VALUES ('%s')";
        add_esc = escape(dbh->db, key->data.s);

        if(add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_DOM:
        sql_tmpl = "INSERT IGNORE INTO domains(domain) VALUES ('%s')";
        add_esc = escape(dbh->db, key->data.s);

        if(add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_IP:
        sql_tmpl = "REPLACE INTO entries "
            "(`ip`, `helo`, `from`, `to`, "
            " `first`, `pass`, `expire`, `bcount`, `pcount`, `greyd_host`) "
            "VALUES "
            "('%s', '', '', '', %lld, %lld, %lld, %d, %d, '%s')";
        add_esc = escape(dbh->db, key->data.s);

        gd = &val->data.gd;
        if(add_esc && asprintf(&sql, sql_tmpl, add_esc, gd->first, gd->pass,
                               gd->expire, gd->bcount, gd->pcount,
                               dbh->greyd_host) == -1)
        {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_TUPLE:
        sql_tmpl = "REPLACE INTO entries "
            "(`ip`, `helo`, `from`, `to`, "
            " `first`, `pass`, `expire`, `bcount`, `pcount`, `greyd_host`) "
            "VALUES "
            "('%s', '%s', '%s', '%s', %lld, %lld, %lld, %d, %d, '%s')";
        gt = &key->data.gt;
        ip_esc   = escape(dbh->db, gt->ip);
        helo_esc = escape(dbh->db, gt->helo);
        from_esc = escape(dbh->db, gt->from);
        to_esc   = escape(dbh->db, gt->to);

        gd = &val->data.gd;
        if(ip_esc && helo_esc && from_esc && to_esc
           && asprintf(&sql, sql_tmpl, ip_esc, helo_esc, from_esc, to_esc,
                       gd->first, gd->pass, gd->expire, gd->bcount,
                       gd->pcount, dbh->greyd_host) == -1)
        {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(ip_esc);
        free(helo_esc);
        free(from_esc);
        free(to_esc);
        break;

    default:
        return GREYDB_ERR;
    }

    res = PQexec(dbh->db, sql);
    if(PQresultStatus(res) != PGRES_COMMAND_OK) {
        i_warning("put postgesql error: %s", PQerrorMessage(dbh->db));
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
    return GREYDB_ERR;
}

extern int
Mod_db_del(DB_handle_T handle, struct DB_key *key)
{
    return GREYDB_ERR;
}

extern void
Mod_db_get_itr(DB_itr_T itr, int types)
{
}

extern void
Mod_db_itr_close(DB_itr_T itr)
{
}

extern int
Mod_db_itr_next(DB_itr_T itr, struct DB_key *key, struct DB_val *val)
{
  return GREYDB_ERR;
}

extern int
Mod_db_itr_replace_curr(DB_itr_T itr, struct DB_val *val)
{
  return GREYDB_ERR;
}

extern int
Mod_db_itr_del_curr(DB_itr_T itr)
{
  return GREYDB_ERR;
}

extern int
Mod_scan_db(DB_handle_T handle, time_t *now, List_T whitelist,
            List_T whitelist_ipv6, List_T traplist, time_t *white_exp)
{
  return GREYDB_ERR;
}

static char
*escape(PGconn *db, const char *str)
{
  char *esc;
  size_t len = strlen(str);
  int *err;

  if((esc = calloc(2 * len + 1, sizeof(char))) == NULL) {
      i_warning("calloc: %s", strerror(errno));
      return NULL;
  }
  PQescapeStringConn(db, esc, str, len, err);

  return esc;
}

static void
populate_key(PGresult *res, struct DB_key *key, int from)
{
}

static void
populate_val(PGresult *res, struct DB_val *val, int from)
{
}
