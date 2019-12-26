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
 * @file   fw_dummy.c
 * @brief  Dummy firewall driver for testing purposes.
 * @author Mikey Austin
 * @date   2014
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "../src/firewall.h"
#include "../src/list.h"

int Mod_fw_open(FW_handle_T handle)
{
    /* noop */
    return 0;
}

void Mod_fw_close(FW_handle_T handle)
{
    /* noop */
    return;
}

int Mod_fw_replace(FW_handle_T handle, const char* set_name, List_T cidrs, short af)
{
    return List_size(cidrs);
}

void Mod_fw_start_log_capture(FW_handle_T handle)
{
    /* noop */
}

void Mod_fw_end_log_capture(FW_handle_T handle)
{
    /* noop */
}

List_T
Mod_fw_capture_log(FW_handle_T handle)
{
    return NULL;
}

int Mod_fw_lookup_orig_dst(FW_handle_T handle, struct sockaddr* src,
    struct sockaddr* proxy, struct sockaddr* orig_dst)
{
    /* Default to the proxy address. */
    memset(orig_dst, 0, sizeof(*orig_dst));
    memcpy(orig_dst, proxy, sizeof(*orig_dst));

    return 0;
}
