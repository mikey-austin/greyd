/**
 * @file   mod.h
 * @brief  Defines the interface for dynamically loaded module management.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef MOD_DEFINED
#define MOD_DEFINED

#include "failures.h"
#include "config_section.h"

/**
 * Load the driver module from the specified configuration section, and
 * return an open handle.
 */
extern void *Mod_open(Config_section_T section, const char *name);

/**
 * Close a previously opened module handle.
 */
extern void Mod_close(void *handle);

/**
 * Fetch a symbol from the module.
 */
extern void *Mod_get(void *handle, const char *sym);

/**
 * Check for module errors and return if defined.
 */
extern char *Mod_error(void);

#endif
