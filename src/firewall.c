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
 * @file   firewall.c
 * @brief  Implements firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "firewall.h"
#include "config_section.h"
#include "mod.h"
#include "list.h"

#include <unistd.h>

extern FW_handle_T
FW_open(Config_T config)
{
    Config_section_T section;
    FW_handle_T handle;

    /* Setup the firewall handle. */
    if((handle = malloc(sizeof(*handle))) == NULL) {
        i_critical("Could not create firewall handle");
    }

    if((section = Config_get_section(config, "firewall")) == NULL) {
        i_critical("Could not find firewall configuration");
    }

    handle->config  = config;
    handle->section = section;
    handle->fwh     = NULL;

    handle->driver = Mod_open(section);

    handle->fw_open = (int (*)(FW_handle_T))
        Mod_get(handle->driver, "Mod_fw_open");
    handle->fw_close = (void (*)(FW_handle_T))
        Mod_get(handle->driver, "Mod_fw_close");
    handle->fw_replace = (int (*)(FW_handle_T, const char *, List_T, short))
        Mod_get(handle->driver, "Mod_fw_replace");
    handle->fw_lookup_orig_dst =
        (int (*)(FW_handle_T, struct sockaddr *, struct sockaddr *, struct sockaddr *))
        Mod_get(handle->driver, "Mod_fw_lookup_orig_dst");
    handle->fw_start_log_capture =
        (void (*)(FW_handle_T)) Mod_get(handle->driver, "Mod_fw_start_log_capture");
    handle->fw_end_log_capture =
        (void (*)(FW_handle_T)) Mod_get(handle->driver, "Mod_fw_end_log_capture");
    handle->fw_capture_log =
        (List_T (*)(FW_handle_T)) Mod_get(handle->driver, "Mod_fw_capture_log");

    if(handle->fw_open(handle) == -1) {
        Mod_close(handle->driver);
        free(handle);
        return NULL;
    }

    return handle;
}

extern void
FW_close(FW_handle_T *handle)
{
    if(handle == NULL || *handle == NULL)
        return;

    (*handle)->fw_close(*handle);
    Mod_close((*handle)->driver);
    free(*handle);
    *handle = NULL;
}

extern int
FW_replace(FW_handle_T handle, const char *set_name, List_T cidrs, short af)
{
    return handle->fw_replace(handle, set_name, cidrs, af);
}

extern int
FW_lookup_orig_dst(FW_handle_T handle, struct sockaddr *src,
                   struct sockaddr *proxy, struct sockaddr *orig_dst)
{
    return handle->fw_lookup_orig_dst(handle, src, proxy, orig_dst);
}

extern void
FW_start_log_capture(FW_handle_T handle)
{
    handle->fw_start_log_capture(handle);
}

extern void
FW_end_log_capture(FW_handle_T handle)
{
    handle->fw_end_log_capture(handle);
}

extern List_T
FW_capture_log(FW_handle_T handle)
{
    return handle->fw_capture_log(handle);
}
