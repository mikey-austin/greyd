/**
 * @file   main_greydb.c
 * @brief  Main function for the greydb program.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "config.h"
#include "greydb.h"
#include "grey.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
    db = DB_open(config);
    
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
