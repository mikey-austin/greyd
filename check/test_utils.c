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
 * @file   test_utils.c
 * @brief  Unit tests for common utility functions.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 50

int main(void)
{
    char email1[SIZE] = "<tesT@Email.OrG";
    char email2[SIZE] = "<tesT2@Email.OrG>";
    char email3[SIZE] = "test3@email.org";
    char email4[SIZE] = "tesT4@Email.OrG>";
    char email5[SIZE] = "test5@email.ORG";
    char email6[SIZE] = "<te<st6\\@e\\>mai\"l.<<ORG>";
    char buf[SIZE];

    TEST_START(6);

    memset(buf, 0, sizeof(buf));
    normalize_email_addr(email1, buf, sizeof(buf));
    TEST_OK(!strcmp(buf, "test@email.org"), "normalize ok");

    memset(buf, 0, sizeof(buf));
    normalize_email_addr(email2, buf, sizeof(buf));
    TEST_OK(!strcmp(buf, "test2@email.org"), "normalize ok");

    memset(buf, 0, sizeof(buf));
    normalize_email_addr(email3, buf, sizeof(buf));
    TEST_OK(!strcmp(buf, "test3@email.org"), "normalize ok");

    memset(buf, 0, sizeof(buf));
    normalize_email_addr(email4, buf, sizeof(buf));
    TEST_OK(!strcmp(buf, "test4@email.org"), "normalize ok");

    memset(buf, 0, sizeof(buf));
    normalize_email_addr(email5, buf, sizeof(buf));
    TEST_OK(!strcmp(buf, "test5@email.org"), "normalize ok");

    memset(buf, 0, sizeof(buf));
    normalize_email_addr(email6, buf, sizeof(buf));
    TEST_OK(!strcmp(buf, "test6@email.org"), "normalize ok");

    TEST_COMPLETE;
}
