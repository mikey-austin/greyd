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

    handle->driver = Mod_open(section, "firewall");

    handle->fw_open = (int (*)(FW_handle_T))
        Mod_get(handle->driver, "Mod_fw_open");
    handle->fw_close = (void (*)(FW_handle_T))
        Mod_get(handle->driver, "Mod_fw_close");
    handle->fw_replace = (int (*)(FW_handle_T, const char *, List_T, short))
        Mod_get(handle->driver, "Mod_fw_replace");
    handle->fw_lookup_orig_dst =
        (int (*)(FW_handle_T, struct sockaddr *, struct sockaddr *, struct sockaddr *))
        Mod_get(handle->driver, "Mod_fw_lookup_orig_dst");
    handle->fw_log_capture_loop =
        (int (*)(FW_handle_T, void (*callback)(char *, void *), void *))
        Mod_get(handle->driver, "Mod_fw_log_capture_loop");

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

extern int
FW_log_capture_loop(FW_handle_T handle, void (*callback)(char *, void *), void *arg)
{
    return handle->fw_log_capture_loop(handle, callback, arg);
}
