/**
 * @file   test_test_framework.c
 * @brief  Unit tests for the greyd test framework.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"

int
main()
{
    struct Test_Plan_T *plan = Test_start(1);

    Test_ok(plan, (1 == 1), "1 equals 1", __FILE__, __LINE__);

    return Test_complete(plan);
}
