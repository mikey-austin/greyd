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
 * @file   config_section.c
 * @brief  Implements the configuration section interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "config_section.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_SEC_INIT_ENTRIES 25 /**< The number of initial hash entries. */

/**
 * The hash entry destroy callback to cleanup config value objects.
 */
static void Config_section_value_destroy(struct Hash_entry *entry);

extern Config_section_T
Config_section_create(const char *name)
{
    Config_section_T section;
    int nlen = strlen(name) + 1;

    section = malloc(sizeof(*section));
    if(section == NULL) {
        errx(1, "Could not create configuration section");
    }

    /* Set the config section name. */
    section->name = malloc(nlen);
    if(section->name == NULL) {
        errx(1, "Could not set configuration section name");
    }
    sstrncpy(section->name, name, nlen);

    /* Initialize the hash table to store the values. */
    section->vars = Hash_create(CONFIG_SEC_INIT_ENTRIES, Config_section_value_destroy);

    return section;
}

extern void
Config_section_destroy(Config_section_T *section)
{
    if(section == NULL || *section == NULL)
        return;

    if((*section)->name) {
        free((*section)->name);
        (*section)->name = NULL;
    }

    Hash_destroy(&((*section)->vars));

    free(*section);
    *section = NULL;
}

extern Config_value_T
Config_section_get(Config_section_T section, const char *varname)
{
    return (Config_value_T) Hash_get(section->vars, varname);
}

extern void
Config_section_delete(Config_section_T section, const char *varname)
{
    Hash_delete(section->vars, varname);
}

extern void
Config_section_set(Config_section_T section, const char *varname, Config_value_T value)
{
    Hash_insert(section->vars, varname, (Config_value_T) value);
}

extern void
Config_section_set_int(Config_section_T section, const char *varname, int value)
{
    Config_value_T new = Config_value_create(CONFIG_VAL_TYPE_INT);
    Config_value_set_int(new, value);
    Config_section_set(section, varname, new);
}

extern void
Config_section_set_str(Config_section_T section, const char *varname, const char *value)
{
    Config_value_T new = Config_value_create(CONFIG_VAL_TYPE_STR);
    Config_value_set_str(new, value);
    Config_section_set(section, varname, new);
}

static void
Config_section_value_destroy(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        Config_value_destroy((Config_value_T *) &(entry->v));
    }
}

extern int
Config_section_get_int(Config_section_T section, const char *varname, int default_int)
{
    Config_value_T val;

    if((val = Config_section_get(section, varname)) == NULL
       || val->type != CONFIG_VAL_TYPE_INT)
    {
        return default_int;
    }
    else {
        return val->v.i;
    }
}

extern char
*Config_section_get_str(Config_section_T section, const char *varname, char *default_str)
{
    Config_value_T val;

    if((val = Config_section_get(section, varname)) == NULL
       || val->type != CONFIG_VAL_TYPE_STR
       || val->v.s == NULL)
    {
        return default_str;
    }
    else {
        return val->v.s;
    }
}

extern List_T
Config_section_get_list(Config_section_T section, const char *varname)
{
    Config_value_T val;

    if((val = Config_section_get(section, varname)) == NULL
       || val->type != CONFIG_VAL_TYPE_LIST
       || val->v.l == NULL)
    {
        return NULL;
    }
    else {
        return val->v.l;
    }
}
