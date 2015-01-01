/**
 * @file   mod.c
 * @brief  Implements the interface for dynamically loaded module management.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#ifdef HAVE_LTDL_H
# include <ltdl.h>
#else
# error This module requires ltdl.h.
#endif

#include "mod.h"

extern void
*Mod_open(Config_section_T section, const char *name)
{
    void *handle = NULL;
    char *mod_path = NULL;

    LTDL_SET_PRELOADED_SYMBOLS();

    if(lt_dlinit() == 0) {
        if(section == NULL)
            i_critical("No %s configuration set", name);

        if((mod_path = Config_section_get_str(section, "driver", NULL)) == NULL)
            i_critical("No %s module configured", name);

        if((handle = lt_dlopen(mod_path)) == NULL)
            i_critical("Could not open %s: %s", mod_path, lt_dlerror());
    }

    return handle;
}

extern void
Mod_close(void *handle)
{
    if(lt_dlinit() == 0) {
        if(handle != NULL)
            lt_dlclose(handle);
        lt_dlexit();
    }
}

extern void
*Mod_get(void *handle, const char *sym)
{
    void *mod_sym;
    const char *error;

    mod_sym = lt_dlsym(handle, sym);
    if((error = Mod_error()) != NULL) {
        i_critical("Could not find symbol %s: %s", sym, error);
    }

    return mod_sym;
}

extern const char
*Mod_error(void)
{
    return lt_dlerror();
}
