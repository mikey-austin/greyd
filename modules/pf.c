/**
 * @file   pf.c
 * @brief  Pluggable PF firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "../config_section.h"
#include "../list.h"
#include "../ip.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define PATH_PFCTL    "/sbin/pfctl"
#define DEFAULT_TABLE "greyd"

int
Mod_fw_replace_networks(Config_section_T section, List_T cidrs)
{
	char *argv[9] = { "pfctl", "-q", "-t", DEFAULT_TABLE, "-T", "replace",
	    "-f" "-", NULL };
    char *netblock, *pfctl_path = PATH_PFCTL;
	static FILE *pf = NULL;
	int pdes[2];
    struct List_entry_T *entry;
    Config_value_T val;
    struct IP_cidr *cidr;

    /*
     * Pull out the custom paths and/or table names from the config section.
     */
    val = Config_section_get(section, "pfctl_path");
    if((pfctl_path = cv_str(val)) == NULL) {
        pfctl_path = PATH_PFCTL;
    }

    val = Config_section_get(section, "table_name");
    if((argv[3] = cv_str(val)) == NULL) {
        argv[3] = DEFAULT_TABLE;
    }

	if(pf == NULL) {
		if(pipe(pdes) != 0)
			return 1;

		switch (fork()) {
		case -1:
			close(pdes[0]);
			close(pdes[1]);
			return 1;

		case 0:
			/* child */
			close(pdes[1]);
			if(pdes[0] != STDIN_FILENO) {
				dup2(pdes[0], STDIN_FILENO);
				close(pdes[0]);
			}
			execvp(pfctl_path, argv);
			_exit(1);
		}

		/* parent */
		close(pdes[0]);
		pf = fdopen(pdes[1], "w");
		if(pf == NULL) {
			close(pdes[1]);
			return 1;
		}
	}

    LIST_FOREACH(cidrs, entry) {
        cidr = List_entry_value(entry);

        if((netblock = IP_cidr_to_str(cidr)) != NULL) {
            fprintf(pf, "%s\n", netblock);
            free(netblock);
        }
    }

    return 0;
}
