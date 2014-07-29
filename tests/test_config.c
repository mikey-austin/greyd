/**
 * @file   test_config.c
 * @brief  Unit tests for config.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../config.h"

#include <string.h>

#define SEC1 "section 1"

#define VAR1 "variable 1"
#define VAR2 "variable 2"

#define VAL1 "value 1"
#define VAL2 123

int
main()
{
    Config_T c;
    Config_section_T s1, s;
    Config_value_T v;

    TEST_START(4);

    c = Config_create();
    TEST_OK((c != NULL), "Config created successfully");

    s1 = Config_section_create(SEC1);
    Config_section_set_str(s1, VAR1, VAL1);
    Config_section_set_int(s1, VAR2, VAL2);

    Config_add_section(c, s1);
    s = Config_get_section(c, SEC1);
    TEST_OK((s != NULL), "Non-null section fetched as expected");
    TEST_OK((strcmp(s->name, SEC1) == 0), "Fetched section matches expected name");

    s = Config_get_section(c, "this section doesn't exist");
    TEST_OK((s == NULL), "Non-existant section fetched as expected");

    Config_destroy(c);

    /*
     * Test the recursive config loading & parsing from a blank config.
     */
    c = Config_create();
    Config_load_file(c, "data/config_test1.conf");

    Config_destroy(c);

    TEST_COMPLETE;
}
