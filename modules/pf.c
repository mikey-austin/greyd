/**
 * @file   pf.c
 * @brief  Pluggable PF firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "../firewall.h"
#include "../config_section.h"
#include "../list.h"
#include "../ip.h"

#include <stdio.h>
#include <unistd.h>

#define PATH_PFCTL    "/sbin/pfctl"
#define DEFAULT_TABLE "greyd"

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
    char *netblock, *pfctl_path = PATH_PFCTL;
	static FILE *pf = NULL;
    struct List_entry *entry;
    struct IP_cidr *cidr;

    /*
     * Pull out the custom paths and/or table names from the config section.
     */
    pfctl_path = Config_section_get_str(section, "pfctl_path", PATH_PFCTL);
    argv[3]    = Config_section_get_str(section, "table_name", DEFAULT_TABLE);

    if((pf = FW_setup_cntl_pipe(pfctl_path, argv)) == NULL) {
        return -1;
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
