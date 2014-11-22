/**
 * @file   fw_dummy.c
 * @brief  Dummy firewall driver for testing purposes.
 * @author Mikey Austin
 * @date   2014
 */

#include "../firewall.h"
#include "../list.h"

#include <stdio.h>

int
Mod_fw_open(FW_handle_T handle)
{
    /* noop */
    return 0;
}

void
Mod_fw_close(FW_handle_T handle)
{
    /* noop */
    return;
}

int
Mod_fw_replace(FW_handle_T handle, const char *set_name, List_T cidrs)
{
    return List_size(cidrs);
}
