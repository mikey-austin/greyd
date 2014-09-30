/**
 * @file   firewall.c
 * @brief  Implements firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "firewall.h"
#include "config_section.h"

#include <dlfcn.h>

static void *FW_load_module(Config_section_T section);

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

extern int
FW_replace_networks(Config_section_T section, List_T cidrs)
{
    char *error;
    void *handle;
    int (*replace_networks)(Config_section_T section, List_T cidrs), ret;

    handle = FW_load_module(section);
    replace_networks = (int (*)(Config_section_T, List_T))
        dlsym(handle, "Mod_fw_replace_networks");

    if((error = dlerror()) != NULL) {
        I_CRIT("Could not find symbol: %s", error);        
    }

    ret = (*replace_networks)(section, cidrs);
    dlclose(handle);

    return ret;
}

extern void
FW_init(Config_section_T section)
{
    char *error;
    void *handle;
    void (*init)(Config_section_T section);

    handle = FW_load_module(section);
    init = (void (*)(Config_section_T)) dlsym(handle, "Mod_fw_init");

    if((error = dlerror()) != NULL) {
        I_CRIT("Could not find symbol: %s", error);        
    }

    (*init)(section);
    dlclose(handle);
}

static void
*FW_load_module(Config_section_T section)
{
    void *handle;
    char *mod_path = NULL;

    if(section == NULL) {
        I_CRIT("No firewall configuration set");
    }

    if((mod_path = Config_section_get_str(section, "driver", NULL)) == NULL) {
        I_CRIT("No firewall module configured");
    }
    
    if((handle = dlopen(mod_path, RTLD_NOW)) == NULL) {
        I_CRIT("Could not open module: %s", dlerror());
    }

    return handle;
}
