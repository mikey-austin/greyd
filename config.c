/**
 * @file   config.c
 * @brief  Implements the application configuration interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "failures.h"
#include "config_section.h"
#include "hash.h"
#include "config.h"
#include "config_parser.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>

#define CONFIG_INIT_SECTIONS 30
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

extern Config_T
Config_create()
{
    Config_T config;

    config = malloc(sizeof(*config));
    if(config == NULL) {
        I_CRIT("Could not create configuration");
    }

    /* Initialize the hash of sections. */
    config->sections = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);
    config->blacklists = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);
    config->whitelists = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);
    config->processed_includes = Hash_create(CONFIG_INIT_INCLUDES, Config_include_destroy_hash);
    config->includes = Queue_create(Config_include_destroy);

    return config;
}

extern void
Config_destroy(Config_T *config)
{
    if(config == NULL || *config == NULL) {
        return;
    }

    Hash_destroy(&((*config)->sections));
    Hash_destroy(&((*config)->blacklists));
    Hash_destroy(&((*config)->whitelists));
    Hash_destroy(&((*config)->processed_includes));
    Queue_destroy(&((*config)->includes));

    free(*config);
    *config = NULL;
}

extern void
Config_add_section(Config_T config, Config_section_T section)
{
    Hash_insert(config->sections, section->name, section);
}

extern void
Config_add_blacklist(Config_T config, Config_section_T section)
{
    Hash_insert(config->blacklists, section->name, section);
}

extern void
Config_add_whitelist(Config_T config, Config_section_T section)
{
    Hash_insert(config->whitelists, section->name, section);
}

extern Config_section_T
Config_get_section(Config_T config, const char *section_name)
{
    return Hash_get(config->sections, section_name);
}

extern Config_section_T
Config_get_blacklist(Config_T config, const char *section_name)
{
    return Hash_get(config->blacklists, section_name);
}

extern Config_section_T
Config_get_whitelist(Config_T config, const char *section_name)
{
    return Hash_get(config->whitelists, section_name);
}

extern void
Config_load_file(Config_T config, char *file)
{
    Lexer_source_T source;
    Lexer_T lexer;
    Config_parser_T parser;
    char *include;
    int *count;

    if(file) {
        source = Lexer_source_create_from_file(file);
        lexer = Config_lexer_create(source);
        parser = Config_parser_create(lexer);

        if(Config_parser_start(parser, config) != CONFIG_PARSER_OK)
            errx(1, "Parse error encountered while processing %s", file);

        /* Clean up the parser & friends. */
        Config_parser_destroy(&parser);

        /* Record this file as being "processed". */
        if((count = malloc(sizeof(*count))) == NULL)
            err(1, "malloc");

        *count = 1;
        Hash_insert(config->processed_includes, file, count);
    }

    while(Queue_size(config->includes) > 0) {
        include = Queue_dequeue(config->includes);
        if(Hash_get(config->processed_includes, include) == NULL)
            Config_load_file(config, include);

        /* Cleanup the dequeued include. */
        free(include);
        include = NULL;
    }
}

extern void
Config_merge(Config_T config, Config_T from)
{
}


extern void
Config_add_include(Config_T config, const char *file)
{
    char *match, *include;
    int len, i;
    glob_t paths;

    /*
     * Treat the supplied file path as a potential glob expansion.
     */
    if((i = glob(file, GLOB_TILDE, NULL, &paths)) != 0) {
        globfree(&paths);
        return;
    }

    /*
     * Loop through each match and check if it has already been processed.
     */
    for(i = 0; i < paths.gl_pathc; i++) {
        if(Hash_get(config->processed_includes, (match = paths.gl_pathv[i])) == NULL) {
            len = strlen(match) + 1;
            if((include = malloc(len)) == NULL) {
                I_CRIT("Could not enqueue matched file %s", match);
            }
            sstrncpy(include, match, len);

            Queue_enqueue(config->includes, include);
        }
    }

    /*
     * Cleanup the paths.
     */
    globfree(&paths);
}

extern char
*Config_get_str(Config_T config, const char *varname,
                const char *section_name, char *default_str)
{
    Config_section_T section;

    if(section_name == NULL)
        section_name = CONFIG_DEFAULT_SECTION;

    if((section = Hash_get(config->sections, section_name)) == NULL)
        return default_str;
    else
        return Config_section_get_str(section, varname, default_str);
}

extern int
Config_get_int(Config_T config, const char *varname,
               const char *section_name, int default_int)
{
    Config_section_T section;

    if(section_name == NULL)
        section_name = CONFIG_DEFAULT_SECTION;

    if((section = Hash_get(config->sections, section_name)) == NULL)
        return default_int;
    else
        return Config_section_get_int(section, varname, default_int);
}

extern List_T
Config_get_list(Config_T config, const char *varname, const char *section_name)
{
    Config_section_T section;

    if(section_name == NULL)
        section_name = CONFIG_DEFAULT_SECTION;

    if((section = Hash_get(config->sections, section_name)) == NULL)
        return NULL;
    else
        return Config_section_get_list(section, varname);
}

extern void
Config_set_int(Config_T config, const char *varname,
               const char *section_name, int value)
{
    Config_section_T section;

    if(section_name == NULL)
        section_name = CONFIG_DEFAULT_SECTION;

    if((section = Hash_get(config->sections, section_name)) == NULL) {
        section = Config_section_create(section_name);
        Config_add_section(config, section);
    }

    Config_section_set_int(section, varname, value);
}

extern void
Config_set_str(Config_T config, const char *varname,
               const char *section_name, char *value)
{
    Config_section_T section;

    if(section_name == NULL)
        section_name = CONFIG_DEFAULT_SECTION;

    if((section = Hash_get(config->sections, section_name)) == NULL) {
        section = Config_section_create(section_name);
        Config_add_section(config, section);
    }

    Config_section_set_str(section, varname, value);
}

static void
Config_section_value_destroy(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        Config_section_destroy((Config_section_T *) &(entry->v));
    }
}

static void
Config_include_destroy_hash(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        free(entry->v);
        entry->v = NULL;
    }
}

static void
Config_include_destroy(void *value)
{
    char *include = (char *) value;

    if(include) {
        free(include);
        include = NULL;
    }
}
