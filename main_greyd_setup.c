/**
 * @file   main_greyd_setup.c
 * @brief  Main function for the greyd-setup program.
 * @author Mikey Austin
 * @date   2014
 */

#define _GNU_SOURCE

#include "failures.h"
#include "utils.h"
#include "config.h"
#include "list.h"
#include "hash.h"
#include "blacklist.h"
#include "spamd_parser.h"
#include "firewall.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

#define DEFAULT_CONFIG "/etc/greyd/greyd.conf"
#define DEFAULT_CURL   "/bin/curl"
#define DEFAULT_MSG    "You have been blacklisted..."
#define PROGNAME       "greyd-setup"
#define MAX_PLEN       1024
#define METHOD_FTP     "ftp"
#define METHOD_HTTP    "http"
#define METHOD_EXEC    "exec"
#define METHOD_FILE    "file"
#define INIT_BL        10

static void usage();
static Spamd_parser_T get_parser(Config_section_T section, Config_T config);
static int open_child(char *file, char **argv);
static int file_get(char *url, char *curl_path);
static void send_blacklist(Blacklist_T blacklist, int greyonly,
                           Config_T config);

/* Global debug variable. */
static int debug = 0;

static void
usage()
{
    fprintf(stderr, "usage: %s [-bDdn] [-c config]\n", PROGNAME);
    exit(1);
}

static int
file_get(char *url, char *curl_path)
{
    char *argv[4];

    if(curl_path == NULL)
        return -1;

    argv[0] = curl_path;
    argv[1] = "-s";
    argv[2] = url;
    argv[3] = NULL;

    if(debug)
       fprintf(stderr, "Getting %s\n", url);

    return (open_child(curl_path, argv));
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

    if(file == NULL)
        return -1;

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
 * the specified method, create a gz lexer source, then construct and
 * return a parser.
 */
static Spamd_parser_T
get_parser(Config_section_T section, Config_T config)
{
    Spamd_parser_T parser = NULL;
    Lexer_T lexer;
    Lexer_source_T source;
    char *method, *file, **ap, **argv, *curl_path, *url;
    int fd, len;
    gzFile gzf;

    /* Extract the method & file variables from the section. */
    method = Config_section_get_str(section, "method", NULL);
    if((file = Config_section_get_str(section, "file", NULL)) == NULL)
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
        section = Config_get_section(config, CONFIG_DEFAULT_SECTION);
        curl_path = Config_section_get_str(section, "curl_path",
                                           DEFAULT_CURL);

        asprintf(&url, "%s://%s", method, file);
        if(url == NULL) {
            I_WARN("Could not create URL");
            return NULL;
        }

        fd = file_get(url, curl_path);
        free(url);
        url = NULL;
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
        free(argv);
        argv = NULL;
    }
    else {
        I_WARN("Unknown method %s", method);
        return NULL;
    }

    /*
     * Now run the appropriate file descriptor through zlib.
     */
    if((gzf = gzdopen(fd, "r")) == NULL) {
        I_WARN("gzdopen");
        return NULL;
    }

    source = Lexer_source_create_from_gz(gzf);
    lexer = Spamd_lexer_create(source);
    parser = Spamd_parser_create(lexer);

    return parser;
}

static void
send_blacklist(Blacklist_T blacklist, int greyonly, Config_T config)
{
    Config_section_T section;
    List_T cidrs;
    int nadded = 0;

    section = Config_get_section(config, "firewall");
    cidrs = Blacklist_collapse(blacklist);

    if(!greyonly &&
       (!section || (nadded = FW_replace_networks(section, cidrs)) < 0))
    {
        I_CRIT("Could not configure firewall");
    }
    else if(debug) {
        fprintf(stderr, "%d entries added to firewall\n", nadded);
    }

    List_destroy(&cidrs);
}

int
main(int argc, char **argv)
{
    int option, dryrun = 0, greyonly = 1, daemonize = 0;
    int bltype, res, count;
    char *config_path = DEFAULT_CONFIG, *list_name, *message;
    Spamd_parser_T parser;
    Config_T config;
    Config_section_T section;
    Blacklist_T blacklist = NULL;
    Config_value_T val;
    List_T lists;
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
            config_path = optarg;
            break;

        default:
            usage();
            break;
        }
    }

    argc -= optind;
    if(argc != 0) {
        usage();
    }

    config = Config_create();
    Config_load_file(config, config_path);

    if(daemonize) {
        daemon(0, 0);
    }

    section = Config_get_section(config, CONFIG_DEFAULT_SECTION);
    val = Config_section_get(section, "lists");
    lists = cv_list(val);
    if(lists == NULL || List_size(lists) == 0) {
        I_ERR("no lists configured in %s", config_path);
    }

    if(!greyonly && !dryrun) {
        section = Config_get_section(config, "firewall");
        FW_init(section);
    }

    /*
     * Loop through lists configured in the configuration.
     */
    LIST_FOREACH(lists, entry) {
        val = List_entry_value(entry);
        if((list_name = cv_str(val)) == NULL)
            continue;

        if((section = Config_get_blacklist(config, list_name))) {
            /*
             * We have a new blacklist. If there was a previous list,
             * send it off and destroy it before creating the new one.
             */
            if(blacklist && !dryrun) {
                send_blacklist(blacklist, greyonly, config);
            }
            Blacklist_destroy(&blacklist);

            message = Config_section_get_str(section, "message", DEFAULT_MSG);
            blacklist = Blacklist_create(list_name, message);
            bltype = BL_TYPE_BLACK;
        }
        else if((section = Config_get_whitelist(config, list_name))
            && blacklist != NULL)
        {
            /*
             * Add this whitelist's entries to the previous blacklist.
             */
            bltype = BL_TYPE_WHITE;
        }
        else {
            continue;
        }

        if((parser = get_parser(section, config)) == NULL) {
            I_WARN("Ignoring list %s", list_name);
            continue;
        }

        /*
         * Parse the list and populate the current blacklist.
         */
        count = blacklist->count;
        res = Spamd_parser_start(parser, blacklist, bltype);
        if(res != SPAMD_PARSER_OK) {
            I_WARN("Blacklist parse error");
        }

        if(debug) {
            fprintf(stderr, "%slist %s %zu entries\n",
                    (bltype == BL_TYPE_BLACK ? "black" : "white"),
                    list_name,
                    ((blacklist->count - count) / 2));
        }

        Spamd_parser_destroy(&parser);
    }

    /*
     * Send the last blacklist and cleanup the various objects.
     */
    if(blacklist && !dryrun) {
        send_blacklist(blacklist, greyonly, config);
    }
    Blacklist_destroy(&blacklist);
    Config_destroy(&config);

    return 0;
}
