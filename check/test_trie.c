/*
 * Copyright (c) 2015 Mikey Austin <mikey@greyd.org>
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
 * @file   test_trie.c
 * @brief  Unit tests for trie interface.
 * @author Mikey Austin
 * @date   2015
 */

#include "test.h"
#include <trie.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    TEST_START(7);

    unsigned char k1[] = { 0xE0 };
    unsigned char k2[] = { 0xF0 };
    unsigned char k3[] = { 0x50 };
    unsigned char k4[] = { 0x30 };
    unsigned char k5[] = { 0x70 };

    struct Trie* t = Trie_create(NULL, 0, NULL);
    Trie_insert(t, k1, 1);
    Trie_insert(t, k2, 1);
    Trie_insert(t, k3, 1);
    Trie_insert(t, k4, 1);
    Trie_insert(t, k5, 1);

    TEST_OK(Trie_contains(t, k1, 1), "key found");
    TEST_OK(Trie_contains(t, k2, 1), "key found");
    TEST_OK(Trie_contains(t, k3, 1), "key found");
    TEST_OK(Trie_contains(t, k4, 1), "key found");
    TEST_OK(Trie_contains(t, k5, 1), "key found");

    unsigned char k6[] = { 0xC0 };
    TEST_OK(!Trie_contains(t, k6, 1), "key not found");
    Trie_insert(t, k6, 1);
    TEST_OK(Trie_contains(t, k6, 1), "key found");

    Trie_destroy(t);

    TEST_COMPLETE;
}
