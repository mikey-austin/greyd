/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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
#include "greyd_config.h"
#include "config_parser.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>

#define CONFIG_INIT_SECTIONS 8
#define CONFIG_INIT_INCLUDES 3

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
        errx(1, "Could not create configuration");
    }

    /* Configs always have sections. */
    config->sections = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);

    /* Optional structures initialized when first used. */
    config->blacklists         = NULL;
    config->whitelists         = NULL;
    config->plugins            = NULL;
    config->processed_includes = NULL;
    config->includes           = NULL;

    return config;
}

extern void
Config_destroy(Config_T *config)
{
    if(config == NULL || *config == NULL) {
        return;
    }

    if((*config)->sections)
        Hash_destroy(&((*config)->sections));
    if((*config)->blacklists)
        Hash_destroy(&((*config)->blacklists));
    if((*config)->whitelists)
        Hash_destroy(&((*config)->whitelists));
    if((*config)->processed_includes)
        Hash_destroy(&((*config)->processed_includes));
    if((*config)->includes)
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
    if(!config->blacklists)
        config->blacklists = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);
    Hash_insert(config->blacklists, section->name, section);
}

extern void
Config_add_whitelist(Config_T config, Config_section_T section)
{
    if(!config->whitelists)
        config->whitelists = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);
    Hash_insert(config->whitelists, section->name, section);
}

extern void
Config_add_plugin(Config_T config, Config_section_T section)
{
    if(!config->plugins)
        config->plugins = Hash_create(CONFIG_INIT_SECTIONS, Config_section_value_destroy);
    Hash_insert(config->plugins, section->name, section);
}

extern Config_section_T
Config_get_section(Config_T config, const char *section_name)
{
    return config->sections
        ? Hash_get(config->sections, section_name) : NULL;
}

extern Config_section_T
Config_get_blacklist(Config_T config, const char *section_name)
{
    return config->blacklists
        ? Hash_get(config->blacklists, section_name) : NULL;
}

extern Config_section_T
Config_get_whitelist(Config_T config, const char *section_name)
{
    return config->whitelists
        ? Hash_get(config->whitelists, section_name) : NULL;
}

extern Config_section_T
Config_get_plugin(Config_T config, const char *section_name)
{
    return config->plugins
        ? Hash_get(config->plugins, section_name) : NULL;
}

extern void
Config_load_file(Config_T config, char *file)
{
    Lexer_source_T source;
    Lexer_T lexer;
    Config_parser_T parser;
    char *include;
    int *count;

    if(!config->processed_includes) {
        config->processed_includes = Hash_create(
            CONFIG_INIT_INCLUDES, Config_include_destroy_hash);
    }

    if(!config->includes)
        config->includes = Queue_create(Config_include_destroy);

    if(file) {
        source = Lexer_source_create_from_file(file);
        lexer = Config_lexer_create(source);
        parser = Config_parser_create(lexer);

        if(Config_parser_start(parser, config) != CONFIG_PARSER_OK)
            errx(1, "Parse error processing %s, line %d col %d",
                 file, parser->lexer->current_line,
                 parser->lexer->current_line_pos);

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
    List_T keys, section_keys;
    struct List_entry *entry, *keys_entry;
    Config_section_T section, from_section;
    Config_value_T from_val, to_val;
    char *key, *section_key;

    if(!config || !from || (keys = Hash_keys(from->sections)) == NULL)
        return;

    LIST_EACH(keys, entry) {
        key = List_entry_value(entry);

        /* Create the section if it doesn't exist. */
        if((section = Hash_get(config->sections, key)) == NULL) {
            section = Config_section_create(key);
            Config_add_section(config, section);
        }

        from_section = Hash_get(from->sections, key);
        if((section_keys = Hash_keys(from_section->vars)) == NULL)
            continue;

        LIST_EACH(section_keys, keys_entry) {
            section_key = List_entry_value(keys_entry);
            from_val = Hash_get(from_section->vars, section_key);
            to_val = Config_value_clone(from_val);
            Config_section_set(section, section_key, to_val);
        }

        List_destroy(&section_keys);
    }

    List_destroy(&keys);
}

extern void
Config_add_include(Config_T config, const char *file)
{
    char *match, *include;
    int len, i;
    glob_t paths;

    if(!config->processed_includes) {
        config->processed_includes = Hash_create(
            CONFIG_INIT_INCLUDES, Config_include_destroy_hash);
    }

    if(!config->includes)
        config->includes = Queue_create(Config_include_destroy);

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
                errx(1, "Could not enqueue matched file %s", match);
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

extern void
Config_delete(Config_T config, const char *varname,
               const char *section_name)
{
    Config_section_T section;

    if(section_name == NULL)
        section_name = CONFIG_DEFAULT_SECTION;

    if((section = Hash_get(config->sections, section_name)) != NULL) {
        Config_section_delete(section, varname);
    }
}

extern void
Config_append_list_str(Config_T config, const char *varname,
                       const char *section_name, const char *str)
{
    Config_section_T section;
    Config_value_T val, new_val;
    List_T list;

    /* Create the section if it doesn't exist. */
    if((section = Config_get_section(config, section_name)) == NULL) {
        section = Config_section_create(section_name);
        Config_add_section(config, section);
    }

    /* Create the new config value. */
    new_val = Config_value_create(CONFIG_VAL_TYPE_STR);
    Config_value_set_str(new_val, str);

    /* Get the list config value, or create it if it doesn't exist. */
    if((val = Config_section_get(section, varname)) == NULL) {
        val = Config_value_create(CONFIG_VAL_TYPE_LIST);
        Config_section_set(section, varname, val);
    }

    list = Config_section_get_list(section, varname);
    List_insert_after(list, new_val);
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
