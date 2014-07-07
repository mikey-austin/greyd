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
    Config_value_T v1, v2;
    const char *str1 = "hello world";

    TEST_START(4);

    v1 = Config_value_create(CONFIG_VAL_TYPE_INT);
    TEST_OK((v1->type = CONFIG_VAL_TYPE_INT), "Can create int config value");

    v2 = Config_value_create(CONFIG_VAL_TYPE_STR);
    TEST_OK((v1->type = CONFIG_VAL_TYPE_STR), "Can create str config value");

    Config_value_set_int(v1, 100);
    TEST_OK((v1->v.i == 100), "Can set int values");

    Config_value_set_str(v2, str1);
    TEST_OK((strcmp(v2->v.s, str1) == 0), "Can set str values");

    Config_value_destroy(v1);
    Config_value_destroy(v2);

    TEST_COMPLETE;
}
