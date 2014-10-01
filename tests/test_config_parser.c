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
    Lexer_source_T cs;
    Lexer_T lexer;
    Config_parser_T parser;
    Config_T c;
    Config_section_T s;
    Config_value_T v, v2;
    struct List_entry_T *curr;
    int tok, ret, i;
    char *include;
    char *src =
        "# This is a test config file\n\n\n"
        "test_var_1    =  12345 # This is a comment \n"
        "# This is another comment followed by a new line \n\n"
        "section test_section_1 {\n\n\n"
        "    test_var_2 = \"long \\\"string\\\"\", # A \"quoted\" string literal\n"
        "    test_var_3 = 12,\n"
        "    test_var_4 = [ 4321, \"a string\" ]\n"
        "} \n"
        "blacklist spews {\n"
        "    test_var_5 = [\n"
        "    222,\n"
        "    \"another string\" ]\n"
        "}\n"
        "whitelist custom {\n"
        "    test_var_6 = [ 1, 2, 3 ],\n"
        "    test_var_7 = 55\n"
        "}\n"
        "include \"data/config_test1.conf\"";

    TEST_START(22);

    cs = Lexer_source_create_from_str(src, strlen(src));
    lexer = Config_lexer_create(cs);
    c = Config_create();

    parser = Config_parser_create(lexer);
    TEST_OK((parser != NULL), "Parser created successfully");

    ret = Config_parser_start(parser, c);
    TEST_OK((ret == CONFIG_PARSER_OK), "Parse completed successfully");

    s = Config_get_section(c, CONFIG_DEFAULT_SECTION);
    v = Config_section_get(s, "test_var_1");
    TEST_OK((cv_int(v) == 12345), "Parsed global section int variable correctly");

    s = Config_get_section(c, "test_section_1");
    TEST_OK((s != NULL), "Section name parsed correctly");

    v = Config_section_get(s, "test_var_2");
    TEST_OK((strcmp(cv_str(v), "long \"string\"") == 0), "Parsed custom section string variable correctly");

    v = Config_section_get(s, "test_var_3");
    TEST_OK((cv_int(v) == 12), "Parsed custom section int variable correctly");

    v = Config_section_get(s, "test_var_4");
    TEST_OK((cv_type(v) == CONFIG_VAL_TYPE_LIST), "Parsed custom section list variable correctly");
    TEST_OK((List_size(cv_list(v)) == 2), "List size is as expected");

    s = Config_get_blacklist(c, "spews");
    TEST_OK((s != NULL), "Blacklist parsed correctly");
    v = Config_section_get(s, "test_var_5");
    TEST_OK((cv_type(v) == CONFIG_VAL_TYPE_LIST),
            "Parsed custom section list variable correctly");
    TEST_OK((List_size(cv_list(v)) == 2), "List size is as expected");

    i = 1;
    LIST_FOREACH(cv_list(v), curr) {
        v2 = List_entry_value(curr);
        switch(i++) {
        case 1:
            TEST_OK((cv_int(v2) == 222),
                    "Integer list value is correct");
            break;

        case 2:
            TEST_OK((strcmp(cv_str(v2), "another string") == 0),
                    "String list value is correct");
            break;
        }
    }

    s = Config_get_whitelist(c, "custom");
    TEST_OK((s != NULL), "Whitelist parsed correctly");
    v = Config_section_get(s, "test_var_6");
    TEST_OK((cv_type(v) == CONFIG_VAL_TYPE_LIST), "Parsed custom section list variable correctly");
    TEST_OK((List_size(cv_list(v)) == 3), "List size is as expected");

    i = 1;
    LIST_FOREACH(cv_list(v), curr) {
        v2 = List_entry_value(curr);
        TEST_OK((cv_int(v2) == i), "List value is correct");
        i++;
    }

    v = Config_section_get(s, "test_var_7");
    TEST_OK((cv_int(v) == 55), "Parsed whitelist section int variable correctly");

    TEST_OK((Queue_size(c->includes) == 1), "Include parsed and enqueued correctly");
    include = (char *) Queue_dequeue(c->includes);
    TEST_OK((include && (strcmp(include, "data/config_test1.conf") == 0)), "Correct included file");

    free(include);
    Config_parser_destroy(&parser);
    Config_destroy(&c);

    TEST_COMPLETE;
}
