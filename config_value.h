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
    union v {
        int   i;
        char *s;
    };
};

extern T    Config_value_create(short type);
extern T    Config_value_set_int(T value, int data);
extern T    Config_value_set_str(T value, const char *data);
extern void Config_value_destroy(T value);

#undef T
#endif
