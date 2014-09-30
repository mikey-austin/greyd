/**
 * @file   ipset.c
 * @brief  Pluggable IPset firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "../firewall.h"
#include "../config_section.h"
#include "../list.h"
#include "../ip.h"

#include <stdio.h>

#define PATH_IPSET  "/usr/sbin/ipset"
#define DEFAULT_SET "greyd"
#define MAX_ELEM    200000
#define HASH_SIZE   (1024 * 1024)

void
Mod_fw_init(Config_section_T section)
{
	char *argv[9] = { "ipset", "-", NULL };
    char *ipset_path = PATH_IPSET, *set_name;
	static FILE *ipset = NULL;
    int max_elem, hash_size;

    /*
     * Pull out the custom paths and/or table names from the config section.
     */
    ipset_path = Config_section_get_str(section, "ipset_path", PATH_IPSET);
    set_name   = Config_section_get_str(section, "table_name", DEFAULT_SET);
    max_elem   = Config_section_get_int(section, "max_elem", MAX_ELEM);
    hash_size  = Config_section_get_int(section, "hash_size", HASH_SIZE);

    if((ipset = FW_setup_cntl_pipe(ipset_path, argv)) == NULL) {
        return;
    }

    /* Flush the set in preparation for the addition of new entries. */
    fprintf(ipset, "create %s hash:net hashsize %d maxelem %d -exist\n",
            set_name, hash_size, max_elem);
    fprintf(ipset, "flush %s\n", set_name);
    fprintf(ipset, "quit\n");
    fclose(ipset);
}

int
Mod_fw_replace_networks(Config_section_T section, List_T cidrs)
{
	char *argv[9] = { "ipset", "-", NULL };
    char *netblock, *ipset_path = PATH_IPSET, *set_name;
	static FILE *ipset = NULL;
	int nadded = 0;
    struct List_entry_T *entry;
    struct IP_cidr *cidr;

    /*
     * Pull out the custom paths and/or table names from the config section.
     */
    ipset_path = Config_section_get_str(section, "ipset_path", PATH_IPSET);
    set_name   = Config_section_get_str(section, "table_name", DEFAULT_SET);

    if((ipset = FW_setup_cntl_pipe(ipset_path, argv)) == NULL) {
        return -1;
    }

    LIST_FOREACH(cidrs, entry) {
        cidr = List_entry_value(entry);

        if((netblock = IP_cidr_to_str(cidr)) != NULL) {
            fprintf(ipset, "add %s %s -exist\n", set_name, netblock);
            nadded++;
            free(netblock);
        }
    }

    fprintf(ipset, "quit\n");
    fclose(ipset);

    return nadded;
}
