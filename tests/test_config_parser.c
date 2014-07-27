/**
 * @file   test_config_lexer.c
 * @brief  Unit tests for the configuration file lexer.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../config_parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main()
{
    Config_source_T cs;
    Config_lexer_T lexer;
    Config_parser_T parser;
    Config_T c;
    Config_section_T s;
    Config_value_T v;
    int tok, ret;

    TEST_START(3);

    cs = Config_source_create_from_str(
        "test_var_1    =  12345 # This is a comment \n"
        "# This is another comment followed by a new line \n\n"
        "section test_section_1 {\n\n\n"
        "    test_var_2 = \"long \\\"string\\\"\", # A \"quoted\" string literal\n"
        "    test_var_3 = 12\n"
        "} \n"
        "include \"/etc/somefile\"");
    lexer = Config_lexer_create(cs);
    c = Config_create();

    parser = Config_parser_create(lexer);
    TEST_OK((parser != NULL), "Parser created successfully");

    ret = Config_parser_start(parser, c);
    TEST_OK((ret == CONFIG_PARSER_OK), "Parse completed successfully");

    s = Config_get_section(c, CONFIG_PARSER_DEFAULT_SECTION);
    v = Config_section_get(s, "test_var_1");
    TEST_OK((v->v.i == 12345), "Parsed global section int variable correctly");

    Config_parser_destroy(parser);
    Config_destroy(c);

    TEST_COMPLETE;
}
