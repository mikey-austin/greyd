/**
 * @file   config_value.h
 * @brief  Defines a configuration variable value.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_VALUE_DEFINED
#define CONFIG_VALUE_DEFINED

#define T Config_value_T

#define CONFIG_VAL_TYPE_INT 1
#define CONFIG_VAL_TYPE_STR 2

/**
 * The structure of a configuration value.
 */
typedef struct T *T;
struct T {
    short type;
    union {
        int   i;
        char *s;
    } v;
};

/**
 * Create a new configuration value structure initialized to the specified type.
 */
extern T    Config_value_create(short type);

/**
 * Set the integer data into the specified value.
 */
extern void Config_value_set_int(T value, int data);

/**
 * Set the string data into the specified value.
 */
extern void Config_value_set_str(T value, const char *data);

/**
 * Destroy a configuration variable, including it's contents if required.
 */
extern void Config_value_destroy(T value);

#undef T
#endif
