/**
 * @file   test_test_framework.c
 * @brief  Unit tests for the greyd test framework.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"

int
main(void)
{
    TEST_START(4);

    TEST_OK((1 == 1), "1 equals 1");
    TEST_OK((1 == 1), "1 equals 1");
    TEST_OK((1 == 1), "1 equals 1");
    TEST_OK((1 == 1), "1 equals 1");

    TEST_COMPLETE;
}
