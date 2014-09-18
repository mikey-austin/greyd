/**
 * @file   test_spamd_parser.c
 * @brief  Unit tests for the spamd file parser.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../spamd_parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main()
{
    Lexer_source_T ls;
    Lexer_T lexer;
    Spamd_parser_T parser;
    Blacklist_T bl;
    int tok, ret;
    char *src =
        "192.168.12.1/32 # This is a test CIDR address\n"
        "10.10.10.0 # A single address\n"
        "192.168.1.1 - 192.168.150.0 # A range\n";

    TEST_START(3);

    ls = Lexer_source_create_from_str(src, strlen(src));
    lexer = Spamd_lexer_create(ls);
    bl = Blacklist_create("Test List", "You have been blacklisted");

    parser = Spamd_parser_create(lexer);
    TEST_OK((parser != NULL), "Parser created successfully");

    ret = Spamd_parser_start(parser, bl, 0);
    TEST_OK((ret == SPAMD_PARSER_OK), "Parse completed successfully");

    TEST_OK((bl->count == 6), "Correct number of addresses added to list");

    Spamd_parser_destroy(parser);
    Blacklist_destroy(bl);

    TEST_COMPLETE;
}
