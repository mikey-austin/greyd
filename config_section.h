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
    Hash_T *vars;  /**< A hash of the variables in this section. */
};

extern T              Config_section_create();
extern Config_value_T Config_section_get(T section, const char *varname);
extern void           Config_section_set(T section, const char *varname, Config_value_T value);
extern void           Config_section_set_int(T section, const char *varname, int value);
extern void           Config_section_set_str(T section, const char *varname, const char *value);
extern void           Config_section_destroy(T section);

#undef T
#endif
