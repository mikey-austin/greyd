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
 * @file   test_hash.c
 * @brief  Unit tests for the generic hash interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <hash.h>
#include <list.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define HASH_SIZE 2

#define TEST_KEY1 "test key 1 - big"
#define TEST_KEY2 "test key 2 - test data"
#define TEST_KEY3 "test key 3 - medium"
#define TEST_KEY4 "test key 4 - larger still"
#define TEST_KEY5 "test key 5 - small"

#define TEST_VAL1 "test value 1"
#define TEST_VAL2 "test value 2"
#define TEST_VAL3 "test value 3"
#define TEST_VAL4 "test value 4"
#define TEST_VAL5 "test value 5"

/* Include the private hash callback functions. */
void destroy(struct Hash_entry *entry);

int
main(void)
{
    Hash_T hash;
    List_T keys;
    struct List_entry *entry;
    char *s, *s1, *s2, *s3, *s4, *s5, *s6, *key;
    int i;

    /* Test hash creation. */
    hash = Hash_create(HASH_SIZE, destroy);

    TEST_START(19);

    /* Create a hash string keys to string values. */
    TEST_OK((hash->size == HASH_SIZE), "Hash size is set correctly");
    TEST_OK((hash->num_entries == 0), "Hash number of entries is zero as expected");

    /* Test insertion of keys in different orders. */
    s1 = malloc(strlen(TEST_VAL1) + 1); strcpy(s1, TEST_VAL1);
    s2 = malloc(strlen(TEST_VAL2) + 1); strcpy(s2, TEST_VAL2);
    s3 = malloc(strlen(TEST_VAL3) + 1); strcpy(s3, TEST_VAL3);
    s4 = malloc(strlen(TEST_VAL4) + 1); strcpy(s4, TEST_VAL4);
    s5 = malloc(strlen(TEST_VAL5) + 1); strcpy(s5, TEST_VAL5);
    s6 = malloc(strlen(TEST_VAL5) + 1); strcpy(s6, TEST_VAL5);

    Hash_insert(hash, TEST_KEY5, s5);
    Hash_insert(hash, TEST_KEY4, s4);
    Hash_insert(hash, TEST_KEY3, s3);

    keys = Hash_keys(hash);
    TEST_OK(List_size(keys) == 3, "Hash keys list size ok");
    i = 0;
    LIST_EACH(keys, entry) {
        key = List_entry_value(entry);
        if(!strcmp(key, TEST_KEY3))
            i++;
        if(!strcmp(key, TEST_KEY4))
            i++;
        if(!strcmp(key, TEST_KEY5))
            i++;
    }
    TEST_OK(i == 3, "All expected keys were in keys list");
    List_destroy(&keys);

    /* Test that the realloc worked. */
    TEST_OK((hash->num_entries == 3), "Hash number of entries is as expected");
    TEST_OK((hash->size == (2 * HASH_SIZE)), "Hash size resized as expected");

    Hash_insert(hash, TEST_KEY2, s2);
    Hash_insert(hash, TEST_KEY1, s1);

    TEST_OK((hash->num_entries == 5), "Hash size is as expected");
    TEST_OK((hash->size == (2 * 2 * HASH_SIZE)), "Hash size resized as expected");

    s = Hash_get(hash, TEST_KEY1);
    TEST_OK((strcmp(s, TEST_VAL1) == 0), "Hash entry 1 inserted correctly");

    s = Hash_get(hash, TEST_KEY2);
    TEST_OK((strcmp(s, TEST_VAL2) == 0), "Hash entry 2 inserted correctly");

    s = Hash_get(hash, TEST_KEY3);
    TEST_OK((strcmp(s, TEST_VAL3) == 0), "Hash entry 3 inserted correctly");

    s = Hash_get(hash, TEST_KEY4);
    TEST_OK((strcmp(s, TEST_VAL4) == 0), "Hash entry 4 inserted correctly");

    s = Hash_get(hash, TEST_KEY5);
    TEST_OK((strcmp(s, TEST_VAL5) == 0), "Hash entry 5 inserted correctly");

    i = hash->num_entries;
    Hash_delete(hash, TEST_KEY5);
    s = Hash_get(hash, TEST_KEY5);
    TEST_OK((s == NULL), "Hash entry 5 deleted correctly");
    TEST_OK(((i - 1) == hash->num_entries), "Hash size is as expected");

    /* Test the ability to overwrite values. */
    i = hash->num_entries;
    Hash_insert(hash, TEST_KEY1, s6);
    TEST_OK((i == hash->num_entries), "No new entry was added as expected");
    s = Hash_get(hash, TEST_KEY1);
    TEST_OK((strcmp(s, TEST_VAL5) == 0), "Hash entry overwritten correctly");

    /* Reset the hash and verify that the last value is no longer present. */
    Hash_reset(hash);
    s = Hash_get(hash, TEST_KEY5);
    TEST_OK((s == NULL), "Hash reset correctly");
    TEST_OK((hash->num_entries == 0), "Hash number of entries is zero as expected");

    /* Destroy the hash. */
    Hash_destroy(&hash);

    TEST_COMPLETE;
}

void
destroy(struct Hash_entry *entry)
{
    /* Free the malloced value. */
    if(entry && entry->v) {
        free(entry->v);
    }
}
