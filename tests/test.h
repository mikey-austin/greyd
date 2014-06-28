/**
 * @file   test.h
 * @brief  Defines a simple unit test framework.
 * @author Mikey Austin
 * @date   2014
 *
 * This module defines an interface for individual unit test programs to call. The format
 * is outputted in a way so that external programs may process the results, such as
 * the perl Test::Harness, for example.
 */

#ifndef TEST_DEFINED
#define TEST_DEFINED

#define T           Test_Plan_T
#define TEST_PASSED 0
#define TEST_FAILED 1

struct T {
    int total;
    int current;
    int succeeded;
    int failed;
};

extern struct T *Test_start(int total);
extern int       Test_complete(struct T *plan);
extern int       Test_ok(struct T *plan, int expr, const char *desc, const char *file, const int line);
extern void      Test_results(struct T *plan);

#undef T
#endif
