/**
 * @file   main_greyd_setup.c
 * @brief  Main function for the greyd-setup program.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "utils.h"
#include "config.h"
#include "list.h"
#include "blacklist.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_CONFIG "/etc/greyd/greyd.conf"
#define PROGNAME       "greyd-setup"
#define MAX_PLEN       1024

static void usage();

static void
usage()
{
	fprintf(stderr, "usage: %s [-bDdn]\n", PROGNAME);
	exit(1);    
}

int
main(int argc, char **argv)
{
    int option, dryrun, debug, greyonly = 1, daemonize = 0;
    char *config_path = DEFAULT_CONFIG, *list_name;
    Config_T config;
    Config_section_T section, prev_bl_section;
    Blacklist_T bl;
    Config_value_T val;
    List_T lists, blacklists;
    struct List_entry_T *entry;

	while((option = getopt(argc, argv, "bdDnc:")) != -1) {
		switch(option) {
		case 'n':
			dryrun = 1;
			break;

		case 'd':
			debug = 1;
			break;

		case 'b':
			greyonly = 0;
			break;

		case 'D':
			daemonize = 1;
			break;

        case 'c':
            config_path = strndup(optarg, MAX_PLEN);
            break;

		default:
			usage();
			break;
		}
	}

    config = Config_create();
    Config_load_file(config, config_path);

    section = Config_get_section(config, CONFIG_DEFAULT_SECTION);
    val = Config_section_get(section, "lists");
    lists = cv_list(val);
    if(lists == NULL || List_size(lists) == 0) {
        I_ERR("no lists configured in %s", config_path);
    }

    /* Loop through lists configured in the configuration. */
    LIST_FOREACH(lists, entry) {
        val = List_entry_value(entry);
        if((list_name = cv_str(val)) == NULL)
            continue;

        if((section = Config_get_blacklist(config, list_name))) {
            /*
             * We have a new blacklist.
             */
            prev_bl_section = section;
        }
        else if((section = Config_get_whitelist(config, list_name))
            && prev_bl_section != NULL)
        {
            /*
             * Add this whitelist's entries to the previous blacklist.
             */
        }
        else {
            continue;
        }
    }

    return 0;
}
