/**
 * @file   test_config_lexer.c
 * @brief  Unit tests for the configuration file lexer.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../config_lexer.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main()
{
    char *test_file;
    Config_source_T cs;
    Config_lexer_T lexer;
    int tok;

    TEST_START(7);

    /*
     * Test the string config source first.
     */
    cs = Config_source_create_from_str("test_var_1    =  12345 # This is a comment");
    lexer = Config_lexer_create(cs);
    TEST_OK((lexer != NULL), "Lexer created successfully");

    TEST_OK(((tok = Config_lexer_next_token(lexer)) == CONFIG_LEXER_TOK_NAME), "Test name token type");
    TEST_OK(!strncmp(lexer->current_value.s, "test_var_1", strlen("test_var_1")), "Test token name value");

    TEST_OK(((tok = Config_lexer_next_token(lexer)) == CONFIG_LEXER_TOK_EQ), "Test equals token type");

    TEST_OK(((tok = Config_lexer_next_token(lexer)) == CONFIG_LEXER_TOK_INT), "Test int token type");
    TEST_OK((lexer->current_value.i == 12345), "Test token int value");

    /* As the comment should be ignored, requesting next token should yield EOF. */
    TEST_OK(((tok = Config_lexer_next_token(lexer)) == 0), "Test comment and EOF");

    Config_lexer_destroy(lexer);

    TEST_COMPLETE;
}
