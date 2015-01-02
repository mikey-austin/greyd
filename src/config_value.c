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
 * @file   config_value.c
 * @brief  Implements configuration variable value.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "config_value.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

/**
 * The destructor for config value lists.
 */
static void Config_value_list_entry_destroy(void *value);

extern Config_value_T
Config_value_create(short type)
{
    Config_value_T new;

    new = malloc(sizeof(*new));
    if(new == NULL) {
        err(1, "malloc");
    }
    else {
        new->type = type;
        switch(new->type) {
        case CONFIG_VAL_TYPE_LIST:
            new->v.l = List_create(Config_value_list_entry_destroy);
            break;
        }
    }

    return new;
}

extern void
Config_value_set_int(Config_value_T value, int data)
{
    value->type = CONFIG_VAL_TYPE_INT;
    value->v.i  = data;
}

extern void
Config_value_set_str(Config_value_T value, const char *data)
{
    int slen = strlen(data) + 1;

    value->type = CONFIG_VAL_TYPE_STR;
    value->v.s = malloc(slen);
    if(value->v.s == NULL)
        err(1, "malloc");
    sstrncpy(value->v.s, data, slen);
}

extern void
Config_value_destroy(Config_value_T *value)
{
    if(value == NULL || *value == NULL)
        return;

    switch((*value)->type) {
    case CONFIG_VAL_TYPE_STR:
        if((*value)->v.s != NULL) {
            free((*value)->v.s);
            (*value)->v.s = NULL;
        }
        break;

    case CONFIG_VAL_TYPE_LIST:
        if((*value)->v.l != NULL) {
            List_destroy((List_T *) &((*value)->v.l));
            (*value)->v.l = NULL;
        }
        break;
    }

    free(*value);
    *value = NULL;
}

extern Config_value_T
Config_value_clone(Config_value_T value)
{
    Config_value_T clone, value_list_entry, entry_clone;
    struct List_entry *entry;

    if(value == NULL)
        return NULL;

    clone = Config_value_create(value->type);
    switch(clone->type) {
    case CONFIG_VAL_TYPE_INT:
        Config_value_set_int(clone, value->v.i);
        break;

    case CONFIG_VAL_TYPE_STR:
        Config_value_set_str(clone, value->v.s);
        break;

    case CONFIG_VAL_TYPE_LIST:
        LIST_FOREACH(value->v.l, entry) {
            value_list_entry = List_entry_value(entry);
            entry_clone = Config_value_clone(value_list_entry);
            List_insert_after(clone->v.l, entry_clone);
        }
        break;
    }

    return clone;
}

extern char
*cv_str(Config_value_T value)
{
    return (value && value->type == CONFIG_VAL_TYPE_STR
            ? value->v.s : NULL);
}

extern int
cv_int(Config_value_T value)
{
    return (value && value->type == CONFIG_VAL_TYPE_INT
            ? value->v.i : -1);
}

extern List_T
cv_list(Config_value_T value)
{
    return (value && value->type == CONFIG_VAL_TYPE_LIST
            ? value->v.l : NULL);
}

extern int
cv_type(Config_value_T value)
{
    return value->type;
}

static void
Config_value_list_entry_destroy(void *value)
{
    Config_value_destroy((Config_value_T *) &value);
}
