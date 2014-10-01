/**
 * @file   config.h
 * @brief  Defines the application configuration interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_DEFINED
#define CONFIG_DEFINED

#include "hash.h"
#include "queue.h"
#include "config_section.h"

#define T Config_T

#define CONFIG_DEFAULT_SECTION "default"

/**
 * The main config table structure.
 */
typedef struct T *T;
struct T {
    Hash_T  sections;           /**< This config's sections */
    Hash_T  blacklists;         /**< Configured blacklists */
    Hash_T  whitelists;         /**< Configured whitelists */
    Hash_T  processed_includes; /**< For tracking processed included files */
    Queue_T includes;           /**< A queue of included files to be parsed */
};

/**
 * Create a new empty configuration object.
 */
extern T Config_create();

/**
 * Destroy a config table, freeing all sections.
 */
extern void Config_destroy(T *config);

/*
 * Add a section to the configuration.
 */
extern void Config_add_section(T config, Config_section_T section);

/*
 * Add a blacklist to the configuration.
 */
extern void Config_add_blacklist(T config, Config_section_T section);

/*
 * Add a whitelist to the configuration.
 */
extern void Config_add_whitelist(T config, Config_section_T section);

/**
 * Get a configuration section by name if one exists.
 */
extern Config_section_T Config_get_section(T config, const char *section_name);

/**
 * Get a blacklist by name if one exists.
 */
extern Config_section_T Config_get_blacklist(T config, const char *section_name);

/**
 * Get a whitelist by name if one exists.
 */
extern Config_section_T Config_get_whitelist(T config, const char *section_name);

/**
 * Parse the specified file and load the configuration data. Any included
 * sub-configuration files will also be parsed.
 */
extern void Config_load_file(T config, char *file);

/**
 * Add the specified file to the list of include files to process if it
 * hasn't been processed already.
 */
extern void Config_add_include(T config, const char *file);

#undef T
#endif
