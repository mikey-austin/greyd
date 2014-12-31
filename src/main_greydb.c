/**
 * @file   main_greydb.c
 * @brief  Main function for the greydb program.
 * @author Mikey Austin
 * @date   2014
 */

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#include "constants.h"
#include "greyd_config.h"
#include "greydb.h"
#include "grey.h"
#include "utils.h"
#include "sync.h"
#include "ip.h"
#include "log.h"

#define PROG_NAME "greydb"

#define TYPE_WHITE    0
#define TYPE_TRAPHIT  1
#define TYPE_SPAMTRAP 2

#define ACTION_LIST 0
#define ACTION_DEL  1
#define ACTION_ADD  2

extern char *optarg;
extern int optind, opterr, optopt;

static void usage(void);
static int db_list(DB_handle_T);
static int db_update(DB_handle_T, char *, int, int, Sync_engine_T,
                     int, int);

static void
usage(void)
{
    fprintf(stderr, "usage: %s [-f config] [[-Tt] -a keys] [[-Tt] -d keys] \n",
            PROG_NAME);
    exit(1);
}

static int
db_list(DB_handle_T db)
{
    DB_itr_T itr;
    struct DB_key key;
    struct DB_val val;
    struct Grey_tuple gt;
    struct Grey_data gd;

    DB_start_txn(db);
    itr = DB_get_itr(db);
    while(DB_itr_next(itr, &key, &val) != GREYDB_NOT_FOUND) {
        gd = val.data.gd;

        switch(key.type) {
        case DB_KEY_TUPLE:
            /*
             * This is a greylist entry.
             */
            gt = key.data.gt;
            printf("GREY|%s|%s|%s|%s|%lld|%lld|%lld|%d|%d\n",
                   gt.ip, gt.helo, gt.from, gt.to, (long long) gd.first,
                   (long long) gd.pass, (long long) gd.expire,
                   gd.bcount, gd.pcount);
            break;

        case DB_KEY_IP:
        case DB_KEY_MAIL:
            /*
             * We have a non-greylist entry.
             */
            switch(gd.pcount) {
            case -1:
                /* Spamtrap hit, with expiry time. */
                printf("TRAPPED|%s|%lld\n", key.data.s,
                       (long long) gd.expire);
                break;

            case -2:
                /* Spamtrap address. */
                printf("SPAMTRAP|%s\n", key.data.s);
                break;

            default:
                /* Must be a whitelist entry. */
                printf("WHITE|%s|||%lld|%lld|%lld|%d|%d\n", key.data.s,
                       (long long) gd.first, (long long) gd.pass,
                       (long long) gd.expire, gd.bcount, gd.pcount);
                break;
            }
            break;
        }
    }

    DB_close_itr(&itr);
    DB_commit_txn(db);

    return 0;
}

static int
db_update(DB_handle_T db, char *ip, int action, int type,
          Sync_engine_T syncer, int white_expiry, int trap_expiry)
{
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;
    char addr[GREY_MAX_MAIL];
    time_t now;
    int ret, i;

    memset(&gd, 0, sizeof(gd));
    now = time(NULL);

    if(action && (type == TYPE_TRAPHIT || type == TYPE_WHITE)) {
        /*
         * We are expecting a numeric IP address.
         */
        if(IP_check_addr(ip) == -1) {
            warnx("Invalid IP address %s", ip);
            return 1;
        }

        key.type = DB_KEY_IP;
    }
    else {
        /*
         * This must be an email address, so ensure it is
         * and normalize.
         */
        key.type = DB_KEY_MAIL;

        if(strchr(ip, '@') == NULL) {
            warnx("Not an email address: %s", ip);
            return 1;
        }

        normalize_email_addr(ip, addr, sizeof(addr));
        ip = addr;
    }

    DB_start_txn(db);

    key.data.s = ip;
    if(action == ACTION_DEL) {
        ret = DB_del(db, &key);
        switch(ret) {
        case GREYDB_NOT_FOUND:
            warnx("No entry for %s", ip);
            goto rollback;

        case GREYDB_ERR:
            warnx("Deletion failed");
            goto rollback;
        }
    }
    else {
        /*
         * Add a new entry.
         */
        ret = DB_get(db, &key, &val);
        switch(ret) {
        case GREYDB_FOUND:
            /*
             * Update the existing entry in the database.
             */
            gd = val.data.gd;
            gd.pcount++;
            switch(type) {
            case TYPE_WHITE:
                gd.pass = now;
                gd.expire = now + white_expiry;
                break;

            case TYPE_TRAPHIT:
                gd.expire = now + trap_expiry;
                gd.pcount = -1;
                break;

            case TYPE_SPAMTRAP:
                gd.expire = 0;
                gd.pcount = -2;
                break;

            default:
                warnx("unknown type %d", type);
                goto rollback;
            }

            val.type = DB_VAL_GREY;
            val.data.gd = gd;
            if((ret = DB_put(db, &key, &val)) != GREYDB_OK) {
                warnx("Put failed");
                goto rollback;
            }
            break;

        case GREYDB_NOT_FOUND:
            /*
             * Create a fresh entry and insert into the database.
             */
            gd.first = now;
            gd.bcount = 1;

            switch(type) {
            case TYPE_WHITE:
                gd.pass = now;
                gd.expire = now + white_expiry;
                break;

            case TYPE_TRAPHIT:
                gd.expire = now + trap_expiry;
                gd.pcount = -1;
                break;

            case TYPE_SPAMTRAP:
                gd.expire = 0;
                gd.pcount = -2;
                break;

            default:
                warnx("Unknown type %d", type);
                goto rollback;
            }

            val.type = DB_VAL_GREY;
            val.data.gd = gd;
            if((ret = DB_put(db, &key, &val)) != GREYDB_OK) {
                warnx("Put failed");
                goto rollback;
            }
            break;

        default:
        case GREYDB_ERR:
            goto rollback;
        }
    }

    DB_commit_txn(db);

    if(syncer && action != ACTION_DEL) {
        switch(type) {
        case TYPE_WHITE:
            Sync_white(syncer, ip, now, gd.expire);
            break;

        case TYPE_TRAPHIT:
            Sync_trapped(syncer, ip, now, gd.expire);
            break;
        }
    }

    return 0;

rollback:
    DB_rollback_txn(db);

    return 1;
}

int
main(int argc, char **argv)
{
    int option, type = TYPE_WHITE, action = ACTION_LIST, i, ret = 0, c = 0;
    char *config_path = DEFAULT_CONFIG;
    Config_T config;
    DB_handle_T db;
    Sync_engine_T syncer = NULL;
    short sync = 0;
    int white_expiry, trap_expiry;

    while((option = getopt(argc, argv, "adtTf:s")) != -1) {
        switch(option) {
        case 'a':
            action = ACTION_ADD;
            break;

        case 'd':
            action = ACTION_DEL;
            break;

        case 't':
            type = TYPE_TRAPHIT;
            break;

        case 'T':
            type = TYPE_SPAMTRAP;
            break;

        case 'f':
            config_path = optarg;
            break;

        case 's':
            sync = 1;
            break;

        default:
            usage();
            break;
        }
    }

    if(action == ACTION_LIST && type != TYPE_WHITE) {
        usage();
    }

    config = Config_create();
    Config_load_file(config, config_path);

    /* Ensure syslog output is disabled. */
    Config_set_int(config, "syslog_enable", NULL, 0);
    Log_setup(config, PROG_NAME);

    /* Setup sync if enabled in configuration file. */
    if(sync && ((syncer = Sync_init(config)) == NULL)) {
        warnx("sync disabled by configuration");
        sync = 0;
    }
    else if(syncer && Sync_start(syncer) == -1) {
        i_warning("could not start sync engine");
        Sync_stop(&syncer);
        sync = 0;
    }

    Config_set_int(config, "drop_privs", NULL, 0);
    db = DB_init(config);
    DB_open(db, (action == ACTION_LIST ? GREYDB_RO : GREYDB_RW));

    switch(action) {
    case ACTION_LIST:
        ret = db_list(db);
        break;

    case ACTION_DEL:
    case ACTION_ADD:
        white_expiry = Config_get_int(config, "white_expiry", "grey", GREY_WHITEEXP);
        trap_expiry = Config_get_int(config, "trap_expiry", "grey", GREY_TRAPEXP);

        for(i = optind; i < argc; i++) {
            if(argv[i][0] != '\0') {
                c++;
                ret += db_update(db, argv[i], action, type, syncer,
                                 white_expiry, trap_expiry);
            }
        }

        if(c == 0) {
            warnx("No addresses specified");
        }
        break;

    default:
        warnx("Bad action specified");
    }

    DB_close(&db);
    Config_destroy(&config);

    return ret;
}
