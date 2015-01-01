/**
 * @file   test_spamd_lexer.c
 * @brief  Unit tests for the spamd lexer.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <spamd_lexer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(void)
{
    Lexer_source_T cs;
    Lexer_T lexer;
    int tok;
    char *src =
        "# This is a comment \n"
        "192.168.12.1              # Trailing comment \n"
        "192.12.2.0 - 192.12.2.255 # IP range \n"
        "114.23.44.22/24           # CIDR notation \n"
        "123455";

    TEST_START(59);

    /*
     * Test the string config source first.
     */
    cs = Lexer_source_create_from_str(src, strlen(src));
    lexer = Spamd_lexer_create(cs);
    TEST_OK((lexer != NULL), "Lexer created successfully");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_EOL), "Test new line");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int token type");
    TEST_OK((lexer->current_value.i == 192), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int token type");
    TEST_OK((lexer->current_value.i == 168), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 12), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 1), "Test token int value");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_EOL), "Test new line");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int token type");
    TEST_OK((lexer->current_value.i == 192), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 12), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 2), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 0), "Test token int value");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DASH), "Test dash");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int token type");
    TEST_OK((lexer->current_value.i == 192), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 12), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 2), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int token type");
    TEST_OK((lexer->current_value.i == 255), "Test token int value");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_EOL), "Test new line");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int token type");
    TEST_OK((lexer->current_value.i == 114), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 23), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int token type");
    TEST_OK((lexer->current_value.i == 44), "Test token int value");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_DOT), "Test dot");
    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 22), "Test token int value");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_SLASH), "Test slash");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int token type");
    TEST_OK((lexer->current_value.i == 24), "Test token int value");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_EOL), "Test new line");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int8 overflow");
    TEST_OK((lexer->current_value.i == 123), "Test token int value");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT8), "Test int8 overflow");
    TEST_OK((lexer->current_value.i == 45), "Test token int value");

    TEST_OK(((tok = Lexer_next_token(lexer)) == SPAMD_LEXER_TOK_INT6), "Test int8 overflow");
    TEST_OK((lexer->current_value.i == 5), "Test token int value");

    Lexer_destroy(&lexer);

    TEST_COMPLETE;
}
