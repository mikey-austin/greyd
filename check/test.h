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

#define T Test_Plan_T
#define TEST_PASSED 0
#define TEST_FAILED 1

#define TEST_START(total) \
    struct Test_Plan_T* __plan = Test_start(total);

#define TEST_OK(expr, desc) \
    Test_ok(__plan, expr, desc, __FILE__, __LINE__);

#define TEST_COMPLETE \
    return Test_complete(__plan);

struct T {
    int total;
    int current;
    int succeeded;
    int failed;
};

/**
 * Initialize a test run with the total number of expected tests to be run.
 */
extern struct T* Test_start(int total);

/**
 * Finish off a test run, and perform and cleanup.
 */
extern int Test_complete(struct T* plan);

/**
 * Test that the supplied expression evaluates to 'true'.
 */
extern void Test_ok(struct T* plan, int expr, const char* desc, const char* file, const int line);

/**
 * Print a summary of the test results.
 */
extern void Test_results(struct T* plan);

/**
 * Construct and return the full filesystem path to the supplied test data filename.
 * The resultant path string must be explicitly freed after use.
 */
extern char* Test_get_file_path(const char* filename);

#undef T
#endif
