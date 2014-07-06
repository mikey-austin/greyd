/**
 * @file   config.h
 * @brief  Defines the application configuration interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_DEFINED
#define CONFIG_DEFINED

#include "hash.h"

#define T Config_T

/**
 * The main config table structure.
 */
typedef struct T *T;
struct T {
    char   *file;      /**< The file making up this configuration. */
    Hash_T *sections;  /**< A hash of the sections comprising this configuration. */
};

/**
 * Create a new empty configuration object.
 */
extern T Config_create();

/**
 * Create a new configuration object from a file.
 */
extern T Config_create_from_file(const char *file);

/**
 * Destroy a config table, freeing all sections.
 */
extern void Config_destroy(T config);

#undef T
#endif
