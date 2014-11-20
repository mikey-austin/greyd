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

int
Mod_fw_open(FW_handle_T handle)
{
    char *argv[3] = { "ipset", "-", NULL };
    char *ipset_path = PATH_IPSET;
    static FILE *ipset = NULL;

    ipset_path = Config_section_get_str(
        handle->section, "ipset_path", PATH_IPSET);

    if((ipset = FW_setup_cntl_pipe(ipset_path, argv)) == NULL) {
        return -1;
    }

    handle->fwh = ipset;
    return 0;
}

void
Mod_fw_close(FW_handle_T handle)
{
    FILE *ipset = handle->fwh;

    if(ipset) {
        fprintf(ipset, "quit\n");
        fclose(ipset);
        handle->fwh = NULL;
    }
}

int
Mod_fw_replace(FW_handle_T handle, const char *set_name, List_T cidrs)
{
    char *netblock;
    FILE *ipset = handle->fwh;
    struct List_entry *entry;
    struct IP_cidr *cidr;
    int nadded = 0, hash_size, max_elem;

    hash_size = Config_section_get_int(
        handle->section, "hash_size", HASH_SIZE);
    max_elem = Config_section_get_int(
        handle->section, "max_elements", MAX_ELEM);

    /*
     * Prepare a new temporary set to populate, then replace the
     * existing set of the specified name with the new set, deleting
     * the old.
     */
    fprintf(ipset, "create temp-%s hash:net hashsize %d maxelem %d -exist\n",
            set_name, hash_size, max_elem);
    fprintf(ipset, "flush temp-%s\n", set_name);

    /* Add the netblocks to the temporary set. */
    LIST_FOREACH(cidrs, entry) {
        cidr = List_entry_value(entry);

        if((netblock = IP_cidr_to_str(cidr)) != NULL) {
            fprintf(ipset, "add temp-%s %s -exist\n", set_name, netblock);
            nadded++;
            free(netblock);
        }
    }

    /* Make sure the desired list exists before swapping. */
    fprintf(ipset, "create %s hash:net hashsize %d maxelem %d -exist\n",
            set_name, hash_size, max_elem);

    fprintf(ipset, "swap %s temp-%s\n", set_name, set_name);
    fprintf(ipset, "destroy temp-%s\n", set_name);

    return nadded;
}
