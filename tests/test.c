/**
 * @file   test.c
 * @brief  Implements the simple unit test framework.
 * @author Mikey Austin
 * @date   2014
 *
 * The output of these testing functions are intended to mock the PERL
 * core Test::Simple module, so that these tests can make use of the PERL core
 * Test::Harness functionality.
 */

#include <stdio.h>
#include <stdlib.h>

#include "test.h"

#define T Test_Plan_T

extern struct T
*Test_start(int total)
{
    struct T *plan;

    plan = (struct T *) malloc(sizeof(*plan));
    if(plan == NULL)
        return NULL;

    /* Initialize the plan. */
    plan->total     = total;
    plan->current   = 1;
    plan->succeeded = 0;
    plan->failed    = 0;

    /* Print a simple header indicating the number of tests to be run. */
    printf("%d..%d\n", plan->current, plan->total);

    return plan;
}

extern void
Test_ok(struct T *plan, int expr, const char *desc, const char *file, const int line)
{
    if(expr) {
        printf("ok %d - %s\n", plan->current, desc);
        plan->succeeded++;
    } else {
        printf("not ok %d - %s\n", plan->current, desc);
        printf("#   Failed test '%s'\n", desc);
        printf("#   at %s line %d.\n", file, line);
        plan->failed++;
    }

    plan->current++;
}

extern void
Test_results(struct T *plan)
{
    if(plan->failed > 0) {
        printf("# Looks like you failed %d test%s of %d.\n",
               plan->failed, (plan->failed > 1 ? "s" : ""), plan->total);
    }
}

extern int
Test_complete(struct T *plan)
{
    int failed = plan->failed;

    /* Print the summary. */
    Test_results(plan);

    /* Clean up. */
    if(plan != NULL) {
        free(plan);
        plan = NULL;
    }

    return failed;
}

