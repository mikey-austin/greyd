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
 * @file   test_config_section.c
 * @brief  Unit tests for config section structures.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <config_section.h>

#include <string.h>

#define SEC1 "section 1"

#define VAR1 "variable 1"
#define VAR2 "variable 2"

#define VAL1 "value 1"
#define VAL2 123

int
main(void)
{
    Config_section_T s1;
    Config_value_T v;

    TEST_START(6);

    s1 = Config_section_create(SEC1);
    TEST_OK((strcmp(s1->name, SEC1) == 0), "Section created successfully");

    /* Test set/get of string variables. */
    Config_section_set_str(s1, VAR1, VAL1);
    v = Config_section_get(s1, VAR1);
    TEST_OK((strcmp(VAL1, v->v.s) == 0), "String value set/get successfully");

    /* Test set/get of int variables. */
    Config_section_set_int(s1, VAR2, VAL2);
    v = Config_section_get(s1, VAR2);
    TEST_OK((v->v.i == VAL2), "Int value set/get successfully");

    /* Test overwriting string value with an int. */
    Config_section_set_int(s1, VAR1, VAL2);
    v = Config_section_get(s1, VAR1);
    TEST_OK((v->v.i == VAL2), "String value overwritten with int successfully");

    /* Test overwriting int value with an string. */
    Config_section_set_str(s1, VAR2, VAL1);
    v = Config_section_get(s1, VAR2);
    TEST_OK((strcmp(VAL1, v->v.s) == 0), "Int value overwritten with string successfully");

    v = Config_section_get(s1, "This var does not exist");
    TEST_OK((v == NULL), "Requesting unset value returns NULL as expected");

    /*
     * Clean up all allocated memory. We don't need to explicitly test here as valgrind
     * will notify us of any unfreed variables.
     */
    Config_section_destroy(&s1);

    TEST_COMPLETE;
}
