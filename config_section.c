/**
 * @file   config_section.c
 * @brief  Implements the configuration section interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
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
Config_section_create(const char *name)
{
    T section;
    int nlen = strlen(name) + 1;

    section = (T) malloc(sizeof(*section));
    if(section == NULL) {
        I_CRIT("Could not create configuration section");
    }

    /* Set the config section name. */
    section->name = (char *) malloc(nlen);
    if(section->name == NULL) {
        I_CRIT("Could not set configuration section name");
    }
    sstrncpy(section->name, name, nlen);

    /* Initialize the hash table to store the values. */
    section->vars = Hash_create(CONFIG_SEC_INIT_ENTRIES, Config_section_value_destroy);

    return section;
}

extern void
Config_section_destroy(T section)
{
    if(!section)
        return;

    if(section->name) {
        free(section->name);
        section->name = NULL;
    }

    Hash_destroy(section->vars);

    free(section);
    section = NULL;
}

extern Config_value_T
Config_section_get(T section, const char *varname)
{
    return (Config_value_T) Hash_get(section->vars, varname);
}

extern void
Config_section_set(T section, const char *varname, Config_value_T value)
{
    Hash_insert(section->vars, varname, (Config_value_T) value);
}

extern void
Config_section_set_int(T section, const char *varname, int value)
{
    Config_value_T new = Config_value_create(CONFIG_VAL_TYPE_INT);
    Config_value_set_int(new, value);
    Config_section_set(section, varname, new);
}

extern void
Config_section_set_str(T section, const char *varname, const char *value)
{
    Config_value_T new = Config_value_create(CONFIG_VAL_TYPE_STR);
    Config_value_set_str(new, value);
    Config_section_set(section, varname, new);
}

static void
Config_section_value_destroy(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        Config_value_destroy(entry->v);
    }
}

extern int
Config_section_get_int(T section, const char *varname, int default_int)
{
    Config_value_T val;
    
    if((val = Config_section_get(section, varname)) == NULL
       || val->type != CONFIG_VAL_TYPE_INT)
    {
        return default_int;
    }
    else {
        return val->v.i;
    }
}

extern char
*Config_section_get_str(T section, const char *varname, char *default_str)
{
    Config_value_T val;
    
    if((val = Config_section_get(section, varname)) == NULL
       || val->type != CONFIG_VAL_TYPE_STR
       || val->v.s == NULL)
    {
        return default_str;
    }
    else {
        return val->v.s;
    }
}

#undef T
