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
 * @file   test_spamd_parser.c
 * @brief  Unit tests for the spamd file parser.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <spamd_parser.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main(void)
{
    Lexer_source_T ls;
    Lexer_T lexer;
    Spamd_parser_T parser;
    Blacklist_T bl;
    int ret;
    char *src =
        "192.168.12.1/32 # This is a test CIDR address\n"
        "10.10.10.0 # A single address\n"
        "192.168.1.1 - 192.168.150.0 # A range\n";

    TEST_START(3);

    ls = Lexer_source_create_from_str(src, strlen(src));
    lexer = Spamd_lexer_create(ls);
    bl = Blacklist_create("Test List", "You have been blacklisted", BL_STORAGE_TRIE);

    parser = Spamd_parser_create(lexer);
    TEST_OK((parser != NULL), "Parser created successfully");

    ret = Spamd_parser_start(parser, bl, 0);
    TEST_OK((ret == SPAMD_PARSER_OK), "Parse completed successfully");

    TEST_OK((bl->count == 6), "Correct number of addresses added to list");

    Spamd_parser_destroy(&parser);
    Blacklist_destroy(&bl);

    TEST_COMPLETE;
}
