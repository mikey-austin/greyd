/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>, Franz Luther
 * Neulist Carroll <franzneulistcarroll@gmail.com>
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
 * @author Franz Luther Neulist Carroll
 * @date   2016
 */

#include <config.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_LIBPQ_FE_H
#include <libpq-fe.h>
#endif

#include "../src/failures.h"
#include "../src/greydb.h"
#include "../src/utils.h"

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "5432"
#define DEFAULT_DB "greyd"

/**
 * The internal driver handle.
 */
struct postgresql_handle {
    PGconn* db;
    char* greyd_host;
    int txn;
    int connected;
};

struct postgresql_itr {
    PGresult* res;
    struct DB_key* curr;
};

static char* escape(PGconn*, const char*);
static void populate_key(PGresult*, struct DB_key*, int, int);
static void populate_val(PGresult*, struct DB_val*, int, int);

extern void
Mod_db_init(DB_handle_T handle)
{
    struct postgresql_handle* dbh;
    char *path, *hostname;
    int ret, flags, uid_changed = 0, len;

    if ((dbh = malloc(sizeof(*dbh))) == NULL)
        i_critical("malloc: %s", strerror(errno));

    handle->dbh = dbh;
    dbh->db = NULL;
    dbh->txn = 0;
    dbh->connected = 0;

    /* Escape and store the hostname in a static buffer. */
    hostname = Config_get_str(handle->config, "hostname", NULL, "");
    if ((dbh->greyd_host = calloc(2 * (len = strlen(hostname)) + 1,
             sizeof(char)))
        == NULL) {
        i_critical("malloc: %s", strerror(errno));
    }
    PQescapeString(dbh->greyd_host, hostname, len);
}

extern void
Mod_db_open(DB_handle_T handle, int flags)
{
    struct postgresql_handle* dbh = handle->dbh;
    char *dbname, *host, *port, *user, *password, *socket, *sql;
    int expand_dbname = 0;

    if (dbh->connected)
        return;

    host = Config_get_str(handle->config, "host", "database", DEFAULT_HOST);
    port = Config_get_str(handle->config, "port", "database", DEFAULT_PORT);
    dbname = Config_get_str(handle->config, "name", "database", DEFAULT_DB);
    socket = Config_get_str(handle->config, "socket", "database", NULL);
    user = Config_get_str(handle->config, "user", "database", NULL);
    password = Config_get_str(handle->config, "pass", "database", NULL);

    const char* db_keywords[7] = { "host", "port", "dbname", "socket", "user",
        "password", NULL };
    const char* db_values[7] = { host, port, dbname, socket, user, password,
        NULL };

    dbh->db = PQconnectdbParams(db_keywords, db_values, expand_dbname);
    if (PQstatus(dbh->db) != CONNECTION_OK) {
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
    struct postgresql_handle* dbh = handle->dbh;

    if (dbh->txn != 0) {
        /* Already in a transaction. */
        return -1;
    }

    PGresult* result = PQexec(dbh->db, "BEGIN");
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        i_warning("start txn failed: %s", PQerrorMessage(dbh->db));
        goto cleanup;
    }
    dbh->txn = 1;
    PQclear(result);
    return 1;

cleanup:
    PQclear(result);
    if (dbh->db) {
        PQfinish(dbh->db);
    }
    exit(1);
}

extern int
Mod_db_commit_txn(DB_handle_T handle)
{
    struct postgresql_handle* dbh = handle->dbh;

    if (dbh->txn != 1) {
        i_warning("cannot commit, not in transaction");
        return -1;
    }

    PGresult* result = PQexec(dbh->db, "END");
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        i_warning("db txn commit failed: %s", PQerrorMessage(dbh->db));
        goto cleanup;
    }
    dbh->txn = 0;
    PQclear(result);
    return 0;

cleanup:
    PQclear(result);
    if (dbh->db) {
        PQfinish(dbh->db);
    }
    exit(1);
}

extern int
Mod_db_rollback_txn(DB_handle_T handle)
{
    struct postgresql_handle* dbh = handle->dbh;

    if (dbh->txn != 1) {
        i_warning("cannot rollback, not in transaction");
        return -1;
    }

    PGresult* result = PQexec(dbh->db, "ROLLBACK");
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        i_warning("db txn rollback failed: %s", PQerrorMessage(dbh->db));
        goto cleanup;
    }
    dbh->txn = 0;
    PQclear(result);
    return 0;

cleanup:
    PQclear(result);
    if (dbh->db) {
        PQfinish(dbh->db);
    }
    exit(1);
}

extern void
Mod_db_close(DB_handle_T handle)
{
    struct postgresql_handle* dbh = handle->dbh;

    if ((dbh = handle->dbh) != NULL) {
        free(dbh->greyd_host);
        if (dbh->db)
            PQfinish(dbh->db);
        free(dbh);
        handle->dbh = NULL;
    }
}

extern int
Mod_db_put(DB_handle_T handle, struct DB_key* key, struct DB_val* val)
{
    struct postgresql_handle* dbh = handle->dbh;
    char *sql = NULL, *sql_tmpl = NULL;
    struct Grey_tuple* gt;
    struct Grey_data* gd;
    unsigned long len;
    char* add_esc = NULL;
    char* ip_esc = NULL;
    char* helo_esc = NULL;
    char* from_esc = NULL;
    char* to_esc = NULL;

    switch (key->type) {
    case DB_KEY_MAIL:
        sql_tmpl = "INSERT INTO spamtraps(\"address\") VALUES ('%s') "
                   "ON CONFLICT (\"address\") DO NOTHING";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_DOM:
        sql_tmpl = "INSERT INTO domains(\"domain\") VALUES ('%s') "
                   "ON CONFLICT (\"domain\") DO NOTHING";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_IP:
        sql_tmpl = "INSERT INTO entries("
                   "\"ip\", \"helo\", \"from\", \"to\", \"first\", "
                   "\"pass\", \"expire\", \"bcount\", \"pcount\", \"greyd_host\") "
                   "VALUES "
                   "('%s', '', '', '', %lld, %lld, %lld, %d, %d, '%s') "
                   "ON CONFLICT (\"ip\", \"helo\", \"from\", \"to\") "
                   "DO UPDATE SET "
                   "\"first\" = EXCLUDED.\"first\", "
                   "\"pass\" = EXCLUDED.\"pass\", "
                   "\"expire\" = EXCLUDED.\"expire\", "
                   "\"bcount\" = EXCLUDED.\"bcount\", "
                   "\"pcount\" = EXCLUDED.\"pcount\", "
                   "\"greyd_host\" = EXCLUDED.\"greyd_host\"";
        add_esc = escape(dbh->db, key->data.s);

        gd = &val->data.gd;
        if (add_esc && asprintf(&sql, sql_tmpl, add_esc, gd->first, gd->pass, gd->expire, gd->bcount, gd->pcount, dbh->greyd_host) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_TUPLE:
        sql_tmpl = "INSERT INTO entries("
                   "\"ip\", \"helo\", \"from\", \"to\", \"first\", "
                   "\"pass\", \"expire\", \"bcount\", \"pcount\", \"greyd_host\") "
                   "VALUES "
                   "('%s', '%s', '%s', '%s', %lld, %lld, %lld, %d, %d, '%s') "
                   "ON CONFLICT (\"ip\", \"helo\", \"from\", \"to\") "
                   "DO UPDATE SET "
                   "\"first\" = EXCLUDED.\"first\", "
                   "\"pass\" = EXCLUDED.\"pass\", "
                   "\"expire\" = EXCLUDED.\"expire\", "
                   "\"bcount\" = EXCLUDED.\"bcount\", "
                   "\"pcount\" = EXCLUDED.\"pcount\", "
                   "\"greyd_host\" = EXCLUDED.\"greyd_host\"";
        gt = &key->data.gt;
        ip_esc = escape(dbh->db, gt->ip);
        helo_esc = escape(dbh->db, gt->helo);
        from_esc = escape(dbh->db, gt->from);
        to_esc = escape(dbh->db, gt->to);

        gd = &val->data.gd;
        if (ip_esc && helo_esc && from_esc && to_esc
            && asprintf(&sql, sql_tmpl, ip_esc, helo_esc, from_esc, to_esc,
                   gd->first, gd->pass, gd->expire, gd->bcount,
                   gd->pcount, dbh->greyd_host)
                == -1) {
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

    PGresult* result = PQexec(dbh->db, sql);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        i_warning("put postgesql error: %s", PQerrorMessage(dbh->db));
        free(sql);
        PQclear(result);
        goto err;
    }
    free(sql);
    PQclear(result);
    return GREYDB_OK;

err:
    DB_rollback_txn(handle);
    return GREYDB_ERR;
}

extern int
Mod_db_get(DB_handle_T handle, struct DB_key* key, struct DB_val* val)
{
    struct postgresql_handle* dbh = handle->dbh;
    char *sql = NULL, *sql_tmpl = NULL;
    struct Grey_tuple* gt;
    unsigned long len;
    int res = GREYDB_NOT_FOUND;
    char* add_esc = NULL;
    char* ip_esc = NULL;
    char* helo_esc = NULL;
    char* from_esc = NULL;
    char* to_esc = NULL;

    switch (key->type) {
    case DB_KEY_DOM_PART:
        sql_tmpl = "SELECT 0, 0, 0, 0, -3 "
                   "FROM domains WHERE '%s' LIKE ('%' || domain)";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_DOM:
        sql_tmpl = "SELECT 0, 0, 0, 0, -3 "
                   "FROM domains WHERE \"domain\"='%s' "
                   "LIMIT 1";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_MAIL:
        sql_tmpl = "SELECT 0, 0, 0, 0, -2 "
                   "FROM spamtraps WHERE \"address\"='%s' "
                   "LIMIT 1";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_IP:
        sql_tmpl = "SELECT \"first\", \"pass\", \"expire\", \"bcount\", "
                   "\"pcount\" "
                   "FROM entries "
                   "WHERE \"ip\"='%s' AND \"helo\"='' AND \"from\"='' AND \"to\"='' "
                   "LIMIT 1";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_TUPLE:
        sql_tmpl = "SELECT \"first\", \"pass\", \"expire\", \"bcount\", "
                   "\"pcount\" "
                   "FROM entries "
                   "WHERE \"ip\"='%s' AND \"helo\"='%s' AND \"from\"='%s' "
                   "AND \"to\"='%s' "
                   "LIMIT 1";
        gt = &key->data.gt;
        ip_esc = escape(dbh->db, gt->ip);
        helo_esc = escape(dbh->db, gt->helo);
        from_esc = escape(dbh->db, gt->from);
        to_esc = escape(dbh->db, gt->to);

        if (ip_esc && helo_esc && from_esc && to_esc
            && asprintf(&sql, sql_tmpl, ip_esc, helo_esc, from_esc,
                   to_esc)
                == -1) {
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

    PGresult* result = NULL;
    unsigned int expected_fields = 5;

    result = PQexec(dbh->db, sql);
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        i_warning("get postgresql error: %s", PQerrorMessage(dbh->db));
        goto err;
    }

    if (PQnfields(result) == expected_fields
        && PQntuples(result) == 1) {
        res = GREYDB_FOUND;
        populate_val(result, val, 0, 0);
    }

err:
    if (sql != NULL)
        free(sql);
    if (result != NULL)
        PQclear(result);
    return res;
}

extern int
Mod_db_del(DB_handle_T handle, struct DB_key* key)
{
    struct postgresql_handle* dbh = handle->dbh;
    char *sql = NULL, *sql_tmpl = NULL;
    struct Grey_tuple* gt;
    unsigned long len;
    char* add_esc = NULL;
    char* ip_esc = NULL;
    char* helo_esc = NULL;
    char* from_esc = NULL;
    char* to_esc = NULL;

    switch (key->type) {
    case DB_KEY_MAIL:
        sql_tmpl = "DELETE FROM spamtraps WHERE \"address\"='%s'";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_DOM:
        sql_tmpl = "DELETE FROM domains WHERE \"domain\"='%s'";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_IP:
        sql_tmpl = "DELETE FROM entries WHERE \"ip\"='%s' "
                   "AND \"helo\"='' AND \"from\"='' AND \"to\"=''";
        add_esc = escape(dbh->db, key->data.s);

        if (add_esc && asprintf(&sql, sql_tmpl, add_esc) == -1) {
            i_warning("postgresql asprintf error");
            goto err;
        }
        free(add_esc);
        break;

    case DB_KEY_TUPLE:
        sql_tmpl = "DELETE FROM entries WHERE \"ip\"='%s' "
                   "AND \"helo\"='%s' AND \"from\"='%s' AND \"to\"='%s' ";
        gt = &key->data.gt;
        ip_esc = escape(dbh->db, gt->ip);
        helo_esc = escape(dbh->db, gt->helo);
        from_esc = escape(dbh->db, gt->from);
        to_esc = escape(dbh->db, gt->to);

        if (ip_esc && helo_esc && from_esc && to_esc
            && asprintf(&sql, sql_tmpl, ip_esc, helo_esc, from_esc,
                   to_esc)
                == -1) {
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

    PGresult* result = PQexec(dbh->db, sql);

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        i_warning("del postgresql error: %s", PQerrorMessage(dbh->db));
        free(sql);
        PQclear(result);
        goto err;
    }
    free(sql);
    PQclear(result);
    return GREYDB_OK;

err:
    DB_rollback_txn(handle);
    return GREYDB_ERR;
}

extern void
Mod_db_get_itr(DB_itr_T itr, int types)
{
    struct postgresql_handle* dbh = itr->handle->dbh;
    struct postgresql_itr* dbi = NULL;
    char *sql_tmpl, *sql = NULL;
    char *entries, *spamtraps, *domains;

    if ((dbi = malloc(sizeof(*dbi))) == NULL) {
        i_warning("malloc: %s", strerror(errno));
        goto err;
    }
    dbi->res = NULL;
    dbi->curr = NULL;
    itr->dbi = dbi;

    entries = (types & DB_ENTRIES) != 0 ? "TRUE" : "FALSE";
    spamtraps = (types & DB_SPAMTRAPS) != 0 ? "TRUE" : "FALSE";
    domains = (types & DB_DOMAINS) != 0 ? "TRUE" : "FALSE";
    sql_tmpl = "SELECT \"ip\", \"helo\", \"from\", \"to\", "
               "\"first\", \"pass\", \"expire\", \"bcount\", \"pcount\" FROM entries "
               "WHERE %s "
               "UNION "
               "SELECT \"address\", '', '', '', 0, 0, 0, 0, -2 FROM spamtraps "
               "WHERE %s "
               "UNION "
               "SELECT \"domain\", '', '', '', 0, 0, 0, 0, -3 FROM domains "
               "WHERE %s";
    if (asprintf(&sql, sql_tmpl, entries, spamtraps, domains) == -1) {
        i_warning("asprintf");
        goto err;
    }

    PGresult* result = PQexec(dbh->db, sql);
    if (sql && PQresultStatus(result) != PGRES_TUPLES_OK) {
        i_warning("get postgresql error: %s", PQerrorMessage(dbh->db));
        free(sql);
        PQclear(result);
        goto err;
    }
    free(sql);

    dbi->res = result;
    itr->size = PQntuples(result);

    return;

err:
    DB_rollback_txn(itr->handle);
    exit(1);
}

extern void
Mod_db_itr_close(DB_itr_T itr)
{
    struct postgresql_handle* dbh = itr->handle->dbh;
    struct postgresql_itr* dbi = itr->dbi;

    if (dbi) {
        dbi->curr = NULL;
        if (dbi->res != NULL)
            PQclear(dbi->res);
        free(dbi);
        itr->dbi = NULL;
    }
}

extern int
Mod_db_itr_next(DB_itr_T itr, struct DB_key* key, struct DB_val* val)
{
    struct postgresql_itr* dbi = itr->dbi;
    PGresult* result = dbi->res;

    itr->current++;
    if (dbi->res && (itr->current < itr->size)) {
        populate_key(result, key, 0, itr->current);
        populate_val(result, val, 4, itr->current);
        dbi->curr = key;
        return GREYDB_FOUND;
    }
    itr->current--;

    return GREYDB_NOT_FOUND;
}

extern int
Mod_db_itr_replace_curr(DB_itr_T itr, struct DB_val* val)
{
    struct postgresql_handle* dbh = itr->handle->dbh;
    struct postgresql_itr* dbi = itr->dbi;
    struct DB_key* key = dbi->curr;

    return DB_put(itr->handle, key, val);
}

extern int
Mod_db_itr_del_curr(DB_itr_T itr)
{
    struct postgresql_itr* dbi = itr->dbi;

    if (dbi && dbi->res && dbi->curr)
        return DB_del(itr->handle, dbi->curr);

    return GREYDB_ERR;
}

extern int
Mod_scan_db(DB_handle_T handle, time_t* now, List_T whitelist,
    List_T whitelist_ipv6, List_T traplist, time_t* white_exp)
{
    struct postgresql_handle* dbh = handle->dbh;
    PGresult* result = NULL;
    char *sql, *sql_tmpl;
    int ret = GREYDB_OK;
    unsigned int size;

    /*
     * Delete expired entries and whitelist appropriate grey entries,
     * by un-setting the tuple fields (to, from, helo), but only if there
     * is not already a conflicting entry with the same IP address
     * (ie an existing trap entry).
     */
    sql_tmpl = "DELETE FROM entries "
               "WHERE \"expire\" <= EXTRACT(EPOCH FROM now()) "
               "AND \"greyd_host\"='%s'";

    if (asprintf(&sql, sql_tmpl, dbh->greyd_host) == -1) {
        i_warning("postgresql asprintf error");
        goto err;
    }

    result = PQexec(dbh->db, sql);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        i_warning("delete postgresql expired entries: %s",
            PQerrorMessage(dbh->db));
        goto err;
    }
    PQclear(result);
    free(sql);
    sql = NULL;

    sql_tmpl = "UPDATE entries "
               "SET \"helo\" = '', \"from\" = '', \"to\" = '', \"expire\" = %lld "
               "FROM entries e LEFT JOIN entries g "
               "ON g.\"ip\" = e.\"ip\" AND g.\"to\" = '' AND g.\"from\" = '' "
               "WHERE e.\"ip\" = entries.\"ip\" AND e.\"helo\" = entries.\"helo\" "
               "AND e.\"from\" = entries.\"from\" AND e.\"to\" = entries.\"to\" "
               "AND e.\"from\" <> '' AND e.\"to\" <> '' AND e.\"pcount\" >= 0 "
               "AND e.\"pass\" <= EXTRACT(EPOCH FROM now()) AND e.\"greyd_host\"='%s' "
               "AND g.\"ip\" IS NULL";

    if (asprintf(&sql, sql_tmpl, *now + *white_exp, dbh->greyd_host) == -1) {
        i_warning("postgresql asprintf error");
        goto err;
    }

    result = PQexec(dbh->db, sql);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        i_warning("update postgresql entries: %s", PQerrorMessage(dbh->db));
        goto err;
    }
    PQclear(result);
    free(sql);
    sql = NULL;

    /* Add greytrap & whitelist entries. */
    sql = "SELECT \"ip\", NULL, NULL FROM entries "
          "WHERE \"to\"='' AND \"from\"='' AND \"ip\" NOT LIKE '%:%' "
          "AND \"pcount\" >= 0 "
          "UNION "
          "SELECT NULL, \"ip\", NULL FROM entries "
          "WHERE \"to\"='' AND \"from\"='' AND \"ip\" LIKE '%:%' "
          "AND \"pcount\" >= 0 "
          "UNION "
          "SELECT NULL, NULL, \"ip\" FROM entries "
          "WHERE \"to\"='' AND \"from\"='' AND \"pcount\" < 0 ";

    result = PQexec(dbh->db, sql);
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        i_warning("postgresql fetch grey/white entries: %s",
            PQerrorMessage(dbh->db));
        goto err;
    }

    size = PQntuples(result);
    if (size > 0) {
        for (int tuple = 0; tuple < size; tuple++) {
            if (!PQgetisnull(result, tuple, 0)) {
                List_insert_after(whitelist,
                    strdup((const char*)
                            PQgetvalue(result, tuple, 0)));
            } else if (!PQgetisnull(result, tuple, 1)) {
                List_insert_after(whitelist_ipv6,
                    strdup((const char*)
                            PQgetvalue(result, tuple, 1)));
            } else if (!PQgetisnull(result, tuple, 2)) {
                List_insert_after(traplist,
                    strdup((const char*)
                            PQgetvalue(result, tuple, 2)));
            }
        }
    }

err:
    if (result)
        PQclear(result);
    return ret;
}

static char* escape(PGconn* db, const char* str)
{
    char* esc;
    size_t len = strlen(str);
    int* err;

    if ((esc = calloc(2 * len + 1, sizeof(char))) == NULL) {
        i_warning("calloc: %s", strerror(errno));
        return NULL;
    }
    PQescapeStringConn(db, esc, str, len, err);

    return esc;
}

static void
populate_key(PGresult* result, struct DB_key* key, int from, int tuple)
{
    struct Grey_tuple* gt;
    static char buf[(INET6_ADDRSTRLEN + 1) + 3 * (GREY_MAX_MAIL + 1)];
    char* buf_p = buf;

    memset(key, 0, sizeof(*key));
    memset(buf, 0, sizeof(buf));

    /*
     * Empty helo, from & to columns indicate a non-grey entry.
     * A pcount of -2 indicates a spamtrap, and a -3 indicates
     * a permitted domain.
     */
    key->type = ((!memcmp(PQgetvalue(result, tuple, 1), "", 1)
                     && !memcmp(PQgetvalue(result, tuple, 2), "", 1)
                     && !memcmp(PQgetvalue(result, tuple, 3), "", 1))
            ? (atoi(PQgetvalue(result, tuple, 8)) == -2
                    ? DB_KEY_MAIL
                    : (atoi(PQgetvalue(result, tuple, 8)) == -3
                            ? DB_KEY_DOM
                            : DB_KEY_IP))
            : DB_KEY_TUPLE);

    if (key->type == DB_KEY_IP) {
        key->data.s = buf_p;
        sstrncpy(buf_p, (const char*)PQgetvalue(result, tuple, from + 0),
            INET6_ADDRSTRLEN + 1);
    } else if (key->type == DB_KEY_MAIL || key->type == DB_KEY_DOM) {
        key->data.s = buf_p;
        sstrncpy(buf_p, (const char*)PQgetvalue(result, tuple, from + 0),
            GREY_MAX_MAIL + 1);
    } else {
        gt = &key->data.gt;
        gt->ip = buf_p;
        sstrncpy(buf_p, (const char*)PQgetvalue(result, tuple, from + 0),
            INET6_ADDRSTRLEN + 1);
        buf_p += INET6_ADDRSTRLEN + 1;

        gt->helo = buf_p;
        sstrncpy(buf_p, (const char*)PQgetvalue(result, tuple, from + 1),
            GREY_MAX_MAIL + 1);
        buf_p += GREY_MAX_MAIL + 1;

        gt->from = buf_p;
        sstrncpy(buf_p, (const char*)PQgetvalue(result, tuple, from + 2),
            GREY_MAX_MAIL + 1);
        buf_p += GREY_MAX_MAIL + 1;

        gt->to = buf_p;
        sstrncpy(buf_p, (const char*)PQgetvalue(result, tuple, from + 3),
            GREY_MAX_MAIL + 1);
        buf_p += GREY_MAX_MAIL + 1;
    }
}

static void
populate_val(PGresult* result, struct DB_val* val, int from, int tuple)
{
    struct Grey_data* gd;

    memset(val, 0, sizeof(*val));
    val->type = DB_VAL_GREY;
    gd = &val->data.gd;
    gd->first = atoi(PQgetvalue(result, tuple, from + 0));
    gd->pass = atoi(PQgetvalue(result, tuple, from + 1));
    gd->expire = atoi(PQgetvalue(result, tuple, from + 2));
    gd->bcount = atoi(PQgetvalue(result, tuple, from + 3));
    gd->pcount = atoi(PQgetvalue(result, tuple, from + 4));
}
