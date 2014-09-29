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

/**
 * Replace the configured network blocks in the firewall with the supplied
 * list of netblocks.
 */
extern int FW_replace_networks(Config_section_T section, List_T cidrs);

#endif
