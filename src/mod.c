/**
 * @file   mod.c
 * @brief  Implements the interface for dynamically loaded module management.
 * @author Mikey Austin
 * @date   2014
 */

#include "mod.h"

extern void
*Mod_open(Config_section_T section, const char *name)
{
    void *handle;
    char *mod_path = NULL;

    if(section == NULL) {
        i_critical("No %s configuration set", name);
    }

    if((mod_path = Config_section_get_str(section, "driver", NULL)) == NULL) {
        i_critical("No %s module configured", name);
    }

    if((handle = dlopen(mod_path, RTLD_NOW)) == NULL) {
        i_critical("Could not open module: %s", dlerror());
    }

    return handle;
}

extern void
Mod_close(void *handle)
{
    dlclose(handle);
}

extern void
*Mod_get(void *handle, const char *sym)
{
    void *mod_sym;
    char *error;

    mod_sym = dlsym(handle, sym);
    if((error = Mod_error()) != NULL) {
        i_critical("Could not find symbol %s: %s", sym, error);
    }

    return mod_sym;
}

extern char
*Mod_error(void)
{
    return dlerror();
}
