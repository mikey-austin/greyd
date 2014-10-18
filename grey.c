/**
 * @file   grey.c
 * @brief  Implements the greylisting management interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "grey.h"
#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#define T Grey_tuple
#define D Grey_data
#define G Greylister_T

#define GREY_DEFAULT_TL_NAME "greyd-greytrap"
#define GREY_DEFAULT_TL_MSG  "\"Your address %A has mailed to spamtraps here\\n\""

static void destroy_address(void *address);
static void sig_term_children(int sig);

G Grey_greylister = NULL;

extern G
Grey_setup(Config_T config)
{
    G greylister = NULL;
    Config_section_T section;

    if((greylister = (G) malloc(sizeof(*greylister))) == NULL) {
        I_CRIT("Could not malloc greylister");
    }

    greylister->config     = config;
    greylister->trap_out   = NULL;
    greylister->grey_in    = NULL;
    greylister->grey_pid   = -1;
    greylister->reader_pid = -1;
    greylister->startup    = -1;

    section = Config_get_section(config, CONFIG_DEFAULT_SECTION);
    if(section) {
        greylister->traplist_name = Config_section_get_str(
            section, "traplist_name", GREY_DEFAULT_TL_NAME);

        greylister->traplist_msg = Config_section_get_str(
            section, "traplist_message", GREY_DEFAULT_TL_MSG);
    }

    greylister->whitelist = List_create(destroy_address);
    greylister->traplist  = List_create(destroy_address);

    return greylister;
}

extern void
Grey_start(G greylister, pid_t grey_pid, FILE *grey_in, FILE *trap_out)
{
    struct sigaction sa;

    // TODO: setup the database and drop privileges.

    greylister->grey_pid = grey_pid;
    greylister->startup  = time(NULL);
    greylister->grey_in  = grey_in;
    greylister->trap_out = trap_out;

    switch((greylister->reader_pid = fork()))
    {
    case -1:
        I_CRIT("Could not fork greyd reader");
        exit(1);

    case 0:
        /*
         * In child. This process has no access to the
         * firewall configuration handle, nor the traplist
         * configuration pipe to the main greyd process.
         */
        fclose(greylister->trap_out);

        // TODO: Close firewall handle here.

        // TODO: Set proc title to "(greyd db update)".

        if(Grey_start_reader(greylister) == -1) {
            I_CRIT("Greyd reader failed to start");
            exit(1);
        }
        _exit(0);
    }

    /*
     * In parent. This process has no access to the grey data being
     * sent from the main greyd process.
     */
    fclose(greylister->grey_in);

    /*
     * Set a global reference to the configured greylister state,
     * to make it available to signal handlers.
     */
    Grey_greylister = greylister;

	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = sig_term_children;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

    // TODO: Set proc title "(greyd fw whitelist update)".

    Grey_start_scanner(greylister);

    exit(1); /* Not-reached. */
}

extern void
Grey_finish(G *greylister)
{
    pid_t grey_pid, reader_pid;

    if(*greylister == NULL)
        return;

    (*greylister)->config = NULL;

    List_destroy(&((*greylister)->whitelist));
    List_destroy(&((*greylister)->traplist));

    if((*greylister)->trap_out != NULL)
        fclose((*greylister)->trap_out);

    if((*greylister)->grey_in != NULL)
        fclose((*greylister)->grey_in);

    grey_pid   = (*greylister)->grey_pid;
    reader_pid = (*greylister)->reader_pid;

    free(*greylister);
    *greylister = NULL;

    if(grey_pid != -1)
        kill(grey_pid, SIGTERM);

    if(reader_pid != -1)
        kill(reader_pid, SIGTERM);
}

extern int
Grey_start_reader(G greylister)
{
    return 0;
}

extern int
Grey_start_scanner(G greylister)
{
    return 0;
}

static void
destroy_address(void *address)
{
    char *to_destroy = (char *) address;

    if(to_destroy != NULL)
        free(to_destroy);
}

static void
sig_term_children(int sig)
{
    if(Grey_greylister) {
        if(Grey_greylister->grey_pid != -1)
            kill(Grey_greylister->grey_pid, SIGTERM);

        if(Grey_greylister->reader_pid != -1)
            kill(Grey_greylister->reader_pid, SIGTERM);
    }

    _exit(1);
}

#undef T
#undef D
#undef G
