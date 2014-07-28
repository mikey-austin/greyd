/**
 * @file   config.c
 * @brief  Implements the application configuration interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "config_section.h"
#include "hash.h"
#include "config.h"

#include <stdlib.h>

#define T Config_T

#define CONFIG_INIT_SECTIONS 10

/**
 * Destructor for configuration section hash entries.
 */
static void Config_section_value_destroy(struct Hash_entry *entry);

/**
 * Destroy each queue value appropriately when the queue is destroyed.
 */
static void Config_include_destroy(void *value);

extern T
Config_create()
{
    T config;

    config = (T) malloc(sizeof(*config));
    if(config == NULL) {
        I_CRIT("Could not create configuration");
    }

    /* Initialize the hash of sections. */
    config->sections = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);
    config->includes = Queue_create(Config_include_destroy);

    return config;
}

extern void
Config_destroy(T config)
{
    if(!config)
        return;

    Hash_destroy(config->sections);
    Queue_destroy(config->includes);

    free(config);
}

extern void
Config_add_section(T config, Config_section_T section)
{
    Hash_insert(config->sections, (const char *) section->name, section);
}

extern Config_section_T
Config_get_section(T config, const char *section_name)
{
    return (Config_section_T) Hash_get(config->sections, section_name);
}

static void
Config_section_value_destroy(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        Config_section_destroy(entry->v);
    }
}

static void
Config_include_destroy(void *value)
{
    char *include = (char *) value;

    if(include) {
        free(include);
    }
}

#undef T
