/**
 * @file   config_section.h
 * @brief  Defines a configuration section.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_SECTION_DEFINED
#define CONFIG_SECTION_DEFINED

#include "hash.h"
#include "config_value.h"

#define T Config_section_T

/**
 * The config section structure.
 */
typedef struct T *T;
struct T {
    char   *name;  /**< The section name. */
    Hash_T  vars;  /**< A hash of the variables in this section. */
};

/**
 * Create a new configuration section with the specified name.
 */
extern T Config_section_create(const char *name);

/**
 * Destroy a configuration section. All configuration variables will also
 * be automatically destroyed.
 */
extern void Config_section_destroy(T section);

/**
 * Request a variable from the section by the varname. NULL will be returned
 * if the variable does not exist.
 */
extern Config_value_T Config_section_get(T section, const char *varname);

/**
 * Set the specified variable and value into the specified section.
 * Overwriting varname is permitted, with the overwritten value safely
 * handled.
 */
extern void Config_section_set(T section, const char *varname,
                               Config_value_T value);

/**
 * Explicitly set an integer config value.
 */
extern void Config_section_set_int(T section, const char *varname, int value);

/**
 * Explicitly set a string config value.
 */
extern void Config_section_set_str(T section, const char *varname,
                                   const char *value);

/**
 * Explicitly get an integer config value. If one does not exist, the default
 * parameter is returned.
 */
extern int Config_section_get_int(T section, const char *varname,
                                  int default_int);

/**
 * Explicitly get a string config value. If one does not exist, the default
 * parameter is returned.
 */
extern char *Config_section_get_str(T section, const char *varname,
                                    char *default_str);

#undef T
#endif
