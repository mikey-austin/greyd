/**
 * @file   firewall.h
 * @brief  Defines the interface for the firewall control.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef FIREWALL_DEFINED
#define FIREWALL_DEFINED

#include "config.h"
#include "list.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * Setup a pipe for communication with firewall control command.
 */
extern FILE *FW_setup_cntl_pipe(char *command, char **argv);

/**
 * Replace the configured network blocks in the firewall with the supplied
 * list of netblocks.
 */
extern int FW_replace_networks(Config_section_T section, List_T cidrs);

/**
 * Initialize the firewall in preparation for population.
 */
extern void FW_init(Config_section_T section);

#endif
