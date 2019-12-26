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
 * @file   test.c
 * @brief  Implements the simple unit test framework.
 * @author Mikey Austin
 * @date   2014
 *
 * The output of these testing functions are intended to mock the PERL
 * core Test::Simple module, so that these tests can make use of the PERL core
 * Test::Harness functionality.
 */

#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define T Test_Plan_T

#define TEST_DATA_DIR "/data/"
#define TEST_CWD 1024

extern struct T* Test_start(int total)
{
    struct T* plan;

    plan = malloc(sizeof(*plan));
    if (plan == NULL)
        return NULL;

    /* Initialize the plan. */
    plan->total = total;
    plan->current = 1;
    plan->succeeded = 0;
    plan->failed = 0;

    /* Print a simple header indicating the number of tests to be run. */
    printf("%d..%d\n", plan->current, plan->total);

    return plan;
}

extern void
Test_ok(struct T* plan, int expr, const char* desc, const char* file, const int line)
{
    if (expr) {
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
Test_results(struct T* plan)
{
    if (plan->failed > 0) {
        printf("# Looks like you failed %d test%s of %d.\n",
            plan->failed, (plan->failed > 1 ? "s" : ""), plan->total);
    }
}

extern int
Test_complete(struct T* plan)
{
    int failed = plan->failed;

    /* Print the summary. */
    Test_results(plan);

    /* Clean up. */
    if (plan != NULL) {
        free(plan);
        plan = NULL;
    }

    return failed;
}

extern char* Test_get_file_path(const char* filename)
{
    int path_len;
    char *path, cwd[TEST_CWD + 1];

    if (getcwd(cwd, TEST_CWD) == NULL) {
        printf("Could not get current working directory...\n");
        exit(1);
    }

    /* Allocate space for path. */
    path_len = strlen(cwd) + strlen(TEST_DATA_DIR) + strlen(filename) + 1;
    if ((path = malloc(path_len)) == NULL) {
        printf("Could not allocate file path...\n");
        exit(1);
    }

    /* Copy the parts into the new path. */
    strncpy(path, cwd, strlen(cwd));
    strncpy(path + strlen(cwd), TEST_DATA_DIR, strlen(TEST_DATA_DIR));
    strncpy(path + strlen(cwd) + strlen(TEST_DATA_DIR), filename, strlen(filename));
    path[path_len - 1] = '\0';

    return path;
}
