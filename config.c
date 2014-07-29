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
#include "config_parser.h"

#include <stdlib.h>
#include <string.h>

#define T Config_T

#define CONFIG_INIT_SECTIONS 20
#define CONFIG_INIT_INCLUDES 50

/**
 * Destructor for configuration section hash entries.
 */
static void Config_section_value_destroy(struct Hash_entry *entry);

/**
 * Destructor for include hash entries.
 */
static void Config_include_destroy_hash(struct Hash_entry *entry);

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
    config->sections           = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);
    config->processed_includes = Hash_create(CONFIG_INIT_INCLUDES, Config_include_destroy_hash);
    config->includes           = Queue_create(Config_include_destroy);

    return config;
}

extern void
Config_destroy(T config)
{
    if(!config)
        return;

    Hash_destroy(config->sections);
    Hash_destroy(config->processed_includes);
    Queue_destroy(config->includes);

    free(config);
}

extern void
Config_add_section(T config, Config_section_T section)
{
    Hash_insert(config->sections, (const char *) section->name, (void *) section);
}

extern Config_section_T
Config_get_section(T config, const char *section_name)
{
    return (Config_section_T) Hash_get(config->sections, section_name);
}

extern void
Config_load_file(T config, char *file)
{
    Config_source_T source;
    Config_lexer_T lexer;
    Config_parser_T parser;
    char *include;
    int *count;

    if(file) {
        source = Config_source_create_from_file(file);
        lexer = Config_lexer_create(source);
        parser = Config_parser_create(lexer);

        if(Config_parser_start(parser, config) != CONFIG_PARSER_OK) {
            I_CRIT("Parse error encountered when processing %s", file);
        }

        /* Clean up the parser & friends. */
        Config_parser_destroy(parser);

        /* Record this file as being "processed". */
        if((count = (int *) malloc(sizeof(*count))) == NULL) {
            I_CRIT("Could not allocate included file count");
        }

        *count = 1;
        Hash_insert(config->processed_includes, file, (void *) count);
    }

    while(config->includes->size > 0) {
        include = (char *) Queue_dequeue(config->includes);
        count = (int *) Hash_get(config->processed_includes, include);

        if(count == NULL) {
            Config_load_file(config, include);
            free(include);
        }
        else {
            /* Increment the number of times this file has been included. */
            *count += 1;
        }
    }
}

extern void
Config_add_include(T config, const char *file)
{
    char *include;
    int len;

    /*
     * Check if it has already been processed.
     */
    if(Hash_get(config->processed_includes, file) == NULL) {
        len = strlen(file);
        if((include = (char *) malloc((sizeof(*include) * len) + 1)) == NULL) {
            I_CRIT("Could not enqueue parsed included file %s", file);
        }

        strncpy(include, file, len);
        include[len] = '\0';
        Queue_enqueue(config->includes, include);
    }
}

static void
Config_section_value_destroy(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        Config_section_destroy(entry->v);
    }
}

static void
Config_include_destroy_hash(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        free(entry->v);
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
