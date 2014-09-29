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

extern int
FW_replace_networks(Config_section_T section, List_T cidrs)
{
    Config_value_T val;
    char *mod_path = NULL, *error;
    void *handle;
    int (*replace_networks)(Config_section_T section, List_T cidrs), ret;

    val = Config_section_get(section, "driver");
    if((mod_path = cv_str(val)) == NULL) {
        I_CRIT("No firewall module configured");
    }
    
    if((handle = dlopen(mod_path, RTLD_NOW)) == NULL) {
        I_CRIT("Could not open module: %s", dlerror());
    }

    replace_networks = (int (*)(Config_section_T, List_T))
        dlsym(handle, "Mod_fw_replace_networks");

    if((error = dlerror()) != NULL) {
        I_CRIT("Could not find symbol: %s", error);        
    }

    ret = (*replace_networks)(section, cidrs);
    dlclose(handle);

    return ret;
}
