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
 * @file   pf.c
 * @brief  Pluggable PF firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include <stdio.h>
#include <unistd.h>

#include <firewall.h>
#include <config_section.h>
#include <list.h>
#include <ip.h>

#define PATH_PFCTL    "/sbin/pfctl"
#define DEFAULT_TABLE "greyd"

/**
 * Setup a pipe for communication with the control command.
 */
FILE *Mod_setup_cntl_pipe(char *command, char **argv);

void
Mod_fw_init(Config_section_T section)
{
    /* NOOP. */
}

int
Mod_fw_replace_networks(Config_section_T section, List_T cidrs)
{
	char *argv[9] = { "pfctl", "-q", "-t", DEFAULT_TABLE, "-T", "replace",
	    "-f" "-", NULL };
    char *cidr, *pfctl_path = PATH_PFCTL;
	static FILE *pf = NULL;
    struct List_entry *entry;

    /*
     * Pull out the custom paths and/or table names from the config section.
     */
    pfctl_path = Config_section_get_str(section, "pfctl_path", PATH_PFCTL);
    argv[3]    = Config_section_get_str(section, "table_name", DEFAULT_TABLE);

    if((pf = Mod_setup_cntl_pipe(pfctl_path, argv)) == NULL) {
        return -1;
    }

    LIST_FOREACH(cidrs, entry) {
        cidr = List_entry_value(entry);
        if(cidr != NULL) {
            fprintf(pf, "%s\n", cidr);
        }
    }

    return 0;
}

FILE
*Mod_setup_cntl_pipe(char *command, char **argv)
{
    int pdes[2];
    FILE *out;

    if(pipe(pdes) != 0)
        return NULL;

    switch(fork()) {
    case -1:
        close(pdes[0]);
        close(pdes[1]);
        return NULL;

    case 0:
        /* Close all output in child. */
        close(pdes[1]);
        close(STDERR_FILENO);
        close(STDOUT_FILENO);
        if(pdes[0] != STDIN_FILENO) {
            dup2(pdes[0], STDIN_FILENO);
            close(pdes[0]);
        }
        execvp(command, argv);
        _exit(1);
    }

    /* parent */
    close(pdes[0]);
    out = fdopen(pdes[1], "w");
    if(out == NULL) {
        close(pdes[1]);
        return NULL;
    }

    return out;
}
