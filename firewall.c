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
        I_CRIT("Could not create firewall handle");
    }

    if((section = Config_get_section(config, "firewall")) == NULL) {
        I_CRIT("Could not find firewall configuration");
    }

    handle->config  = config;
    handle->section = section;
    handle->fwh     = NULL;

    handle->driver = Mod_open(section, "firewall");

    handle->fw_open = (int (*)(FW_handle_T))
        Mod_get(handle->driver, "Mod_fw_open");
    handle->fw_close = (void (*)(FW_handle_T))
        Mod_get(handle->driver, "Mod_fw_close");
    handle->fw_replace = (int (*)(FW_handle_T, const char *, List_T))
        Mod_get(handle->driver, "Mod_fw_replace");
    handle->fw_lookup_orig_dst =
        (int (*)(FW_handle_T, struct sockaddr *, struct sockaddr *, struct sockaddr *))
        Mod_get(handle->driver, "Mod_fw_lookup_orig_dst");

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
FW_replace(FW_handle_T handle, const char *set_name, List_T cidrs)
{
    return handle->fw_replace(handle, set_name, cidrs);
}

extern int
FW_lookup_orig_dst(FW_handle_T handle, struct sockaddr *src,
                   struct sockaddr *proxy, struct sockaddr *orig_dst)
{
    return handle->fw_lookup_orig_dst(handle, src, proxy, orig_dst);
}

extern FILE
*FW_setup_cntl_pipe(char *command, char **argv)
{
    int pdes[2];
    FILE *out;

    if(pipe(pdes) != 0)
        return NULL;

    switch(fork()) {
    case -1:
        close(pdes[0]);
        close(pdes[1]);
        return NULL;

    case 0:
        /* Close all output in child. */
        close(pdes[1]);
        close(STDERR_FILENO);
        close(STDOUT_FILENO);
        if(pdes[0] != STDIN_FILENO) {
            dup2(pdes[0], STDIN_FILENO);
            close(pdes[0]);
        }
        execvp(command, argv);
        _exit(1);
    }

    /* parent */
    close(pdes[0]);
    out = fdopen(pdes[1], "w");
    if(out == NULL) {
        close(pdes[1]);
        return NULL;
    }

    return out;
}
