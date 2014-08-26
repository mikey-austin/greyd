/**
 * @file   test_lexer_source.c
 * @brief  Unit tests for config source.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../lexer_source.h"

#include <string.h>
#include <stdlib.h>

int
main()
{
    char *test_file, *src = "limit = 10";
    Lexer_source_T cs;
    int c;

    TEST_START(16);

    /*
     * Test the string config source first.
     */
    cs = Lexer_source_create_from_str(src, strlen(src));
    TEST_OK((cs != NULL), "String config source created successfully");

    TEST_OK(((c = Lexer_source_getc(cs)) == 'l'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'm'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful getc");

    Lexer_source_ungetc(cs, c);
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful ungetc");
    TEST_OK(((c = Lexer_source_getc(cs)) == ' '), "Successful getc");

    Lexer_source_destroy(cs);

    /*
     * Test the file config source.
     */
    test_file = Test_get_file_path("lexer_source_1.conf");
    cs = Lexer_source_create_from_file(test_file);
    TEST_OK((cs != NULL), "File config source created successfully");

    TEST_OK(((c = Lexer_source_getc(cs)) == 'l'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'm'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful getc");

    Lexer_source_ungetc(cs, c);
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful ungetc");
    TEST_OK(((c = Lexer_source_getc(cs)) == ' '), "Successful getc");

    Lexer_source_destroy(cs);
    free(test_file);

    TEST_COMPLETE;
}
