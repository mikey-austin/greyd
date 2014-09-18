/**
 * @file   test_config_value.c
 * @brief  Unit tests for config value structures.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../config_value.h"

#include <string.h>

int
main()
{
    Config_value_T v1, v2, v3;
    const char *str1 = "hello world";

    TEST_START(6);

    v1 = Config_value_create(CONFIG_VAL_TYPE_INT);
    TEST_OK((v1->type = CONFIG_VAL_TYPE_INT), "Can create int config value");

    v2 = Config_value_create(CONFIG_VAL_TYPE_STR);
    TEST_OK((v1->type = CONFIG_VAL_TYPE_STR), "Can create str config value");

    Config_value_set_int(v1, 100);
    TEST_OK((cv_int(v1) == 100), "Can set int values");

    Config_value_set_str(v2, str1);
    TEST_OK((strcmp(cv_str(v2), str1) == 0), "Can set str values");

    /*
     * Create a value of list type, and add v1 & v2.
     */
    v3 = Config_value_create(CONFIG_VAL_TYPE_LIST);
    TEST_OK((v3->type = CONFIG_VAL_TYPE_LIST), "Can create list config value");

    List_insert_after(v3->v.l, (void *) v1);
    List_insert_after(v3->v.l, (void *) v2);
    TEST_OK((List_size(v3->v.l) == 2), "Config variables added onto list correctly");

    /*
     * This will destroy v1 & v2 as well.
     */
    Config_value_destroy(v3);

    TEST_COMPLETE;
}
