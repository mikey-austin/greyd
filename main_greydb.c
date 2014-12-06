/**
 * @file   main_greydb.c
 * @brief  Main function for the greydb program.
 * @author Mikey Austin
 * @date   2014
 */

#include "config.h"
#include "greydb.h"
#include "grey.h"
#include "ip.h"

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#define DEFAULT_CONFIG "/etc/greyd/greyd.conf"
#define PROGNAME       "greydb"

/* Types. */
#define TYPE_WHITE    0
#define TYPE_TRAPHIT  1
#define TYPE_SPAMTRAP 2

/* Actions. */
#define ACTION_LIST 0
#define ACTION_DEL  1
#define ACTION_ADD  2

static void usage();
static int db_list(DB_handle_T db);
static int db_update(DB_handle_T db, char *ip, int action, int type);

static void
usage()
{
    fprintf(stderr, "usage: %s [-f config] [[-Tt] -a keys] [[-Tt] -d keys] \n",
            PROGNAME);
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

    return 0;
}

static int
db_update(DB_handle_T db, char *ip, int action, int type)
{
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;
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
         * This must be an email address, so ensure it is and convert to
         * lowercase.
         */
        key.type = DB_KEY_MAIL;

        if(strchr(ip, '@') == NULL) {
            warnx("Not an email address: %s", ip);
            return 1;
        }

        for(i = 0; ip[i] != '\0'; i++) {
            if(isupper((unsigned char) ip[i])) {
                ip[i] = tolower((unsigned char) ip[i]);
            }
        }
    }

    key.data.s = ip;
    if(action == ACTION_DEL) {
        ret = DB_del(db, &key);
        switch(ret) {
        case GREYDB_NOT_FOUND:
            warnx("No entry for %s", ip);
            return 1;

        case GREYDB_ERR:
            warnx("Deletion failed");
            return 1;
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
            if(val.type != DB_VAL_GREY) {
                /* Not expecting this, so delete it. */
                DB_del(db, &key);
                return 1;
            }

            gd = val.data.gd;
            gd.pcount++;
            switch(type) {
            case TYPE_WHITE:
                gd.pass = now;
                gd.expire = now + GREY_WHITEEXP;
                break;

            case TYPE_TRAPHIT:
                gd.expire = now + GREY_TRAPEXP;
                gd.pcount = -1;
                break;

            case TYPE_SPAMTRAP:
                gd.expire = 0;
                gd.pcount = -2;
                break;

            default:
                warnx("unknown type %d", type);
                return 1;
            }

            val.type = DB_VAL_GREY;
            val.data.gd = gd;
            if((ret = DB_put(db, &key, &val)) != GREYDB_OK) {
                warnx("Put failed");
                return 1;
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
                gd.expire = now + GREY_WHITEEXP;
                break;

            case TYPE_TRAPHIT:
                gd.expire = now + GREY_TRAPEXP;
                gd.pcount = -1;
                break;

            case TYPE_SPAMTRAP:
                gd.expire = 0;
                gd.pcount = -2;
                break;

            default:
                warnx("Unknown type %d", type);
                return 1;
            }

            val.type = DB_VAL_GREY;
            val.data.gd = gd;
            if((ret = DB_put(db, &key, &val)) != GREYDB_OK) {
                warnx("Put failed");
                return 1;
            }
            break;

        default:
        case GREYDB_ERR:
            return 1;
        }
    }

    return 0;
}

int
main(int argc, char **argv)
{
    int option, type = TYPE_WHITE, action = ACTION_LIST, i, ret = 0, c = 0;
    char *config_path = DEFAULT_CONFIG;
    Config_T config;
    DB_handle_T db;

    while((option = getopt(argc, argv, "adtTf:")) != -1) {
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
    db = DB_open(config, (action == ACTION_LIST ? GREYDB_RO : GREYDB_RW));

    switch(action) {
    case ACTION_LIST:
        ret = db_list(db);
        break;

    case ACTION_DEL:
    case ACTION_ADD:
        for(i = optind; i < argc; i++) {
            if(argv[i][0] != '\0') {
                c++;
                ret += db_update(db, argv[i], action, type);
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
