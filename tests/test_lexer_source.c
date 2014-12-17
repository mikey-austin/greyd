/**
 * @file   test_lexer_source.c
 * @brief  Unit tests for lexer source.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <lexer_source.h>

#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include <fcntl.h>

int
main(void)
{
    char *test_file, *src = "limit = 10";
    Lexer_source_T cs;
    int c, fd;
    gzFile gzf;

    TEST_START(32);

    /*
     * Test the string lexer source first.
     */
    cs = Lexer_source_create_from_str(src, strlen(src));
    TEST_OK((cs != NULL), "String lexer source created successfully");

    TEST_OK(((c = Lexer_source_getc(cs)) == 'l'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'm'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful getc");

    Lexer_source_ungetc(cs, c);
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful ungetc");
    TEST_OK(((c = Lexer_source_getc(cs)) == ' '), "Successful getc");

    Lexer_source_destroy(&cs);

    /*
     * Test the file lexer source.
     */
    test_file = Test_get_file_path("lexer_source_1.conf");
    cs = Lexer_source_create_from_file(test_file);
    TEST_OK((cs != NULL), "File lexer source created successfully");

    TEST_OK(((c = Lexer_source_getc(cs)) == 'l'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'm'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful getc");

    Lexer_source_ungetc(cs, c);
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful ungetc");
    TEST_OK(((c = Lexer_source_getc(cs)) == ' '), "Successful getc");

    Lexer_source_destroy(&cs);
    free(test_file);

    /*
     * Test the file lexer source, created from a file descriptor.
     */
    test_file = Test_get_file_path("lexer_source_1.conf");
    fd = open(test_file, 0);
    cs = Lexer_source_create_from_fd(fd);
    TEST_OK((cs != NULL), "File descriptor lexer source created successfully");

    TEST_OK(((c = Lexer_source_getc(cs)) == 'l'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'm'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful getc");

    Lexer_source_ungetc(cs, c);
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful ungetc");
    TEST_OK(((c = Lexer_source_getc(cs)) == ' '), "Successful getc");

    Lexer_source_destroy(&cs);
    free(test_file);

    /*
     * Test the gzipped file lexer source.
     */
    test_file = Test_get_file_path("lexer_source_2.conf.gz");
    gzf = gzopen(test_file, "r");
    cs = Lexer_source_create_from_gz(gzf);
    TEST_OK((cs != NULL), "Gzipped lexer source created successfully");

    TEST_OK(((c = Lexer_source_getc(cs)) == 'l'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'm'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 'i'), "Successful getc");
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful getc");

    Lexer_source_ungetc(cs, c);
    TEST_OK(((c = Lexer_source_getc(cs)) == 't'), "Successful ungetc");
    TEST_OK(((c = Lexer_source_getc(cs)) == ' '), "Successful getc");

    Lexer_source_destroy(&cs);
    free(test_file);

    TEST_COMPLETE;
}
