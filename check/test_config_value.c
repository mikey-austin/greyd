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
 * @file   test_config_value.c
 * @brief  Unit tests for config value structures.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <config_value.h>

#include <string.h>

int
main(void)
{
    Config_value_T v1, v2, v3, val;
    Config_value_T c1, c2, c3;
    const char *str1 = "hello world";
    struct List_entry *entry;

    TEST_START(13);

    v1 = Config_value_create(CONFIG_VAL_TYPE_INT);
    TEST_OK((v1->type == CONFIG_VAL_TYPE_INT), "Can create int config value");

    v2 = Config_value_create(CONFIG_VAL_TYPE_STR);
    TEST_OK((v2->type == CONFIG_VAL_TYPE_STR), "Can create str config value");

    Config_value_set_int(v1, 100);
    TEST_OK((cv_int(v1) == 100), "Can set int values");

    Config_value_set_str(v2, str1);
    TEST_OK((strcmp(cv_str(v2), str1) == 0), "Can set str values");

    c1 = Config_value_clone(v1);
    c2 = Config_value_clone(v2);
    TEST_OK((c1 != v1 && c1->type == CONFIG_VAL_TYPE_INT), "int clone type ok");
    TEST_OK((c2 != v2 && c2->type == CONFIG_VAL_TYPE_STR), "str clone type value");
    TEST_OK((cv_int(c1) == 100), "int clone data ok");
    TEST_OK((v2->v.s != c2->v.s && strcmp(cv_str(c2), str1) == 0),
            "str clone data ok");

    /*
     * Create a value of list type, and add v1 & v2.
     */
    v3 = Config_value_create(CONFIG_VAL_TYPE_LIST);
    TEST_OK((v3->type = CONFIG_VAL_TYPE_LIST), "Can create list config value");

    List_insert_after(v3->v.l, (void *) v1);
    List_insert_after(v3->v.l, (void *) v2);
    TEST_OK((List_size(v3->v.l) == 2), "Config variables added onto list correctly");

    c3 = Config_value_clone(v3);
    TEST_OK(c3 != v3 && c3->v.l != v3->v.l &&
            (List_size(v3->v.l) == 2), "List cloned correctly");

    LIST_FOREACH(v3->v.l, entry) {
        val = List_entry_value(entry);
        switch(val->type) {
        case CONFIG_VAL_TYPE_INT:
            TEST_OK((cv_int(val) == 100), "int list clone data ok");
            break;

        case CONFIG_VAL_TYPE_STR:
            TEST_OK((strcmp(cv_str(c2), str1) == 0), "str clone list data ok");
            break;
        }
    }

    /*
     * This will destroy list members as well.
     */
    Config_value_destroy(&v3);
    Config_value_destroy(&c1);
    Config_value_destroy(&c2);
    Config_value_destroy(&c3);

    TEST_COMPLETE;
}
