/**
 * @file   fw_dummy.c
 * @brief  Dummy firewall driver for testing purposes.
 * @author Mikey Austin
 * @date   2014
 */

#include "../firewall.h"
#include "../list.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

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

int
Mod_fw_lookup_orig_dst(FW_handle_T handle, struct sockaddr *src,
                       struct sockaddr *proxy, struct sockaddr *orig_dst)
{
    /* Default to the proxy address. */
    memset(orig_dst, 0, sizeof(*orig_dst));
    memcpy(orig_dst, proxy, sizeof(*orig_dst));

    return 0;
}
