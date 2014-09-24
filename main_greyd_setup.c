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
#include "spamd_parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEFAULT_CONFIG "/etc/greyd/greyd.conf"
#define PROGNAME       "greyd-setup"
#define MAX_PLEN       1024
#define METHOD_FTP     "ftp"
#define METHOD_HTTP    "http"
#define METHOD_EXEC    "exec"
#define METHOD_FILE    "file"

static void usage();
static Spamd_parser_T get_parser(Config_section_T section);
static int open_child(char *file, char **argv);

static void
usage()
{
	fprintf(stderr, "usage: %s [-bDdn]\n", PROGNAME);
	exit(1);    
}

/**
 * Open a pipe, for the specified child process and return the descriptor
 * pertaining to it's stdout.
 */
static int
open_child(char *file, char **argv)
{
	int pdes[2];

	if(pipe(pdes) != 0) 
		return (-1);

	switch(fork()) {
	case -1:
		close(pdes[0]);
		close(pdes[1]);
		return (-1);

	case 0:
		/* child */
		close(pdes[0]);
		if(pdes[1] != STDOUT_FILENO) {
			dup2(pdes[1], STDOUT_FILENO);
			close(pdes[1]);
		}
		execvp(file, argv);
		_exit(1);
	}

	/* parent */
	close(pdes[1]);

	return (pdes[0]);
}

/**
 * Given the relevant configuration section, fetch the blacklist via
 * specified method, decompress into a buffer, then construct and
 * return a parser.
 */
static Spamd_parser_T
get_parser(Config_section_T section)
{
    Spamd_parser_T parser = NULL;
    Config_value_T val;
    char *method, *file, *deflated, **ap, **argv;
    int fd, len;
    
    /* Extract the method & file variables from the section. */
    val = Config_section_get(section, "method");
    method = cv_str(val);

    if((val = Config_section_get(section, "file")) == NULL
       || (file = cv_str(val)) == NULL)
    {
        I_WARN("No file configuration variables set");
        return NULL;
    }

    if((method == NULL)
       || (strncmp(method, METHOD_FILE, strlen(METHOD_FILE)) == 0))
    {
        /*
         * A file on the local filesystem is to be processed.
         */
        fd = open(file, O_RDONLY);
    }
    else if((strncmp(method, METHOD_HTTP, strlen(METHOD_HTTP)) == 0)
            || (strncmp(method, METHOD_FTP, strlen(METHOD_FTP)) == 0))
    {
        /*
         * The file is to be fetched via curl.
         */
    }
    else if(strncmp(method, METHOD_EXEC, strlen(METHOD_EXEC)) == 0) {
        /*
         * The file is to be exec'ed, with the output to be parsed. The
         * string specified in the "file" variable is to be interpreted as
         * a command invocation.
         */
		len = strlen(file);
		if((argv = calloc(len, sizeof(char *))) == NULL) {
			I_ERR("malloc failed");
        }

		for(ap = argv; ap < &argv[len - 1] &&
                (*ap = strsep(&file, " \t")) != NULL;)
        {
			if(**ap != '\0')
				ap++;
		}
		*ap = NULL;
        fd = open_child(argv[0], argv);
    }
    else {
        I_WARN("Unknown method %s", method);
        return NULL;
    }

    /*
     * Now run the appropriate file descriptor through zlib
     */
    deflated = NULL;

    return parser;
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

    /*
     * Cleanup the various objects.
     */
    Config_destroy(config);

    return 0;
}
