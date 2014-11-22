/**
 * @file   config_value.h
 * @brief  Defines a configuration variable value.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_VALUE_DEFINED
#define CONFIG_VALUE_DEFINED

#include "list.h"

#define CONFIG_VAL_TYPE_INT  1
#define CONFIG_VAL_TYPE_STR  2
#define CONFIG_VAL_TYPE_LIST 3  /**< The list type manages a list of config values. */

/**
 * The structure of a configuration value.
 */
typedef struct Config_value_T *Config_value_T;
struct Config_value_T {
    short type;
    union {
        int     i;
        char   *s;
        List_T  l;
    } v;
};

/**
 * Create a new configuration value structure initialized to the specified type.
 */
extern Config_value_T Config_value_create(short type);

/**
 * Set the integer data into the specified value.
 */
extern void Config_value_set_int(Config_value_T value, int data);

/**
 * Set the string data into the specified value.
 */
extern void Config_value_set_str(Config_value_T value, const char *data);

/**
 * Destroy a configuration variable, including it's contents if required.
 */
extern void Config_value_destroy(Config_value_T *value);

/**
 * Accessor functions to hide the implementation of the config values.
 */
extern char  *cv_str(Config_value_T value);
extern int    cv_int(Config_value_T value);
extern List_T cv_list(Config_value_T value);
extern int    cv_type(Config_value_T value);

#endif
