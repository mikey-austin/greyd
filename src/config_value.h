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
 * @file   config_value.h
 * @brief  Defines a configuration variable value.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_VALUE_DEFINED
#define CONFIG_VALUE_DEFINED

#include "list.h"

#define CONFIG_VAL_TYPE_INT 1
#define CONFIG_VAL_TYPE_STR 2
#define CONFIG_VAL_TYPE_LIST 3 /**< The list type manages a list of config values. */

/**
 * The structure of a configuration value.
 */
typedef struct Config_value_T* Config_value_T;
struct Config_value_T {
    short type;
    union {
        int i;
        char* s;
        List_T l;
    } v;
};

/**
 * Create a new configuration value structure initialized to the specified type.
 */
extern Config_value_T Config_value_create(short type);

/**
 * Create and return a clone of the supplied value.
 */
extern Config_value_T Config_value_clone(Config_value_T value);

/**
 * Set the integer data into the specified value.
 */
extern void Config_value_set_int(Config_value_T value, int data);

/**
 * Set the string data into the specified value.
 */
extern void Config_value_set_str(Config_value_T value, const char* data);

/**
 * Destroy a configuration variable, including it's contents if required.
 */
extern void Config_value_destroy(Config_value_T* value);

/**
 * Accessor functions to hide the implementation of the config values.
 */
extern char* cv_str(Config_value_T value);
extern int cv_int(Config_value_T value);
extern List_T cv_list(Config_value_T value);
extern int cv_type(Config_value_T value);

#endif
