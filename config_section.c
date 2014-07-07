/**
 * @file   config_section.c
 * @brief  Implements the configuration section interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "config_section.h"

#include <stdlib.h>
#include <string.h>

#define T Config_section_T

#define CONFIG_SEC_INIT_ENTRIES 25 /**< The number of initial hash entries. */

/**
 * The hash entry destroy callback to cleanup config value objects.
 */
static void Config_section_value_destroy(struct Hash_entry *entry);

extern T
Config_section_create()
{
    T section;

    section = (T) malloc(sizeof(*section));
    if(section == NULL) {
        I_CRIT("Could not create configuration section");
    }

    /* Initialize the hash table to store the values. */
    return NULL;
}

extern Config_value_T
Config_section_get(T section, const char *varname)
{
    return NULL;
}

extern void
Config_section_set(T section, const char *varname, Config_value_T value)
{
    
}

extern void
Config_section_set_int(T section, const char *varname, int value)
{
}

extern void
Config_section_set_str(T section, const char *varname, const char *value)
{
}

extern void
Config_section_destroy(T section)
{
}

static void
Config_section_value_destroy(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        Config_value_destroy(entry->v);
    }
}

#undef T
