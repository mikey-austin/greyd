/**
 * @file   main_greydb.c
 * @brief  Main function for the greydb program.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "failures.h"
#include "config.h"
#include "greydb.h"
#include "grey.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
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
	fprintf(stderr, "usage: %s [[-Tt] -a keys] [[-Tt] -d keys] "
                    "[-c config]\n", PROGNAME);
	exit(1);    
}

static int
db_list(DB_handle_T db)
{
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
    struct addrinfo hints, *res;

    memset(&key, 0, sizeof(key));
    memset(&val, 0, sizeof(val));
    memset(&gd, 0, sizeof(gd));

    now = time(NULL);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST;
    if(action && (type == TYPE_TRAPHIT || type == TYPE_WHITE)) {
        /*
         * We are expecting a numeric IP address.
         */
        if(getaddrinfo(ip, NULL, &hints, &res) != 0) {
            I_WARN("Invalid IP address %s", ip);
            return 1;
        }
        freeaddrinfo(res);

        key.type = DB_KEY_IP;
    }
    else {
        /*
         * This must be an email address, so ensure it is and convert to
         * lowercase.
         */
        key.type = DB_KEY_MAIL;

        if(strchr(ip, '@') == NULL) {
            I_ERR("Not an email address: %s", ip);
            return 1;
        }

        for(i = 0; ip[i] != '\0'; i++) {
            if(isupper((unsigned char) ip[i])) {
                ip[i] = tolower((unsigned char) ip[i]);
            }
        }
    }

    sstrncpy(key.data.s, ip, (strlen(ip) + 1));
    if(action == ACTION_DEL) {
        ret = DB_del(db, &key);
        switch(ret) {
        case GREYDB_NOT_FOUND:
            I_WARN("No entry for %s", ip);
            return 1;

        case GREYDB_ERR:
            I_WARN("Deletion failed");
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
                I_ERR("unknown type %d", type);
            }

            val.data.gd = gd;
            if((ret = DB_put(db, &key, &val)) != GREYDB_OK) {
                I_WARN("Put failed");
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
                I_ERR("Unknown type %d", type);
                return 1;
            }

            val.data.gd = gd;
            if((ret = DB_put(db, &key, &val)) != GREYDB_OK) {
                I_WARN("Put failed");
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
    int option, type = TYPE_WHITE, action = ACTION_LIST, i, r = 0, c = 0;
    char *config_path = DEFAULT_CONFIG;
    Config_T config;
    DB_handle_T db;

	while((option = getopt(argc, argv, "adtTc:")) != -1) {
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

        case 'c':
            config_path = optarg;

		default:
			usage();
			break;
		}
	}

	argc -= optind;
	if (action == ACTION_LIST && type != TYPE_WHITE) {
		usage();
    }

    config = Config_create();
    Config_load_file(config, config_path);
    db = DB_open(config, (action == ACTION_LIST ? GREYDB_RO : GREYDB_RW));
    
    switch(action) {
    case ACTION_LIST:
        return db_list(db);

    case ACTION_DEL:
    case ACTION_ADD:
        for(i = 0; i < argc; i++) {
            if(argv[i][0] != '\0') {
                c++;
                r += db_update(db, argv[i], action, type);
            }
        }
        
        if(c == 0) {
            I_CRIT("No addresses specified");
        }
        break;

    default:
        I_CRIT("Bad action specified");
    }

    DB_close(&db);
    Config_destroy(&config);

    return r;
}
