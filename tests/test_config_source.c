/**
 * @file   test_config_source.c
 * @brief  Unit tests for config source.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../config_source.h"

#include <string.h>
#include <stdlib.h>

int
main()
{
    char *test_file;
    Config_source_T cs;
    int c;

    TEST_START(16);

    /*
     * Test the string config source first.
     */
    cs = Config_source_create_from_str("limit = 10");
    TEST_OK((cs != NULL), "String config source created successfully");

    TEST_OK(((c = Config_source_getc(cs)) == 'l'), "Successful getc"); 
    TEST_OK(((c = Config_source_getc(cs)) == 'i'), "Successful getc"); 
    TEST_OK(((c = Config_source_getc(cs)) == 'm'), "Successful getc"); 
    TEST_OK(((c = Config_source_getc(cs)) == 'i'), "Successful getc"); 
    TEST_OK(((c = Config_source_getc(cs)) == 't'), "Successful getc"); 

    Config_source_ungetc(cs, c);
    TEST_OK(((c = Config_source_getc(cs)) == 't'), "Successful ungetc"); 
    TEST_OK(((c = Config_source_getc(cs)) == ' '), "Successful getc"); 

    Config_source_destroy(cs);

    /*
     * Test the file config source.
     */
    test_file = Test_get_file_path("config_source_1.conf");
    cs = Config_source_create_from_file(test_file);
    TEST_OK((cs != NULL), "File config source created successfully");

    TEST_OK(((c = Config_source_getc(cs)) == 'l'), "Successful getc"); 
    TEST_OK(((c = Config_source_getc(cs)) == 'i'), "Successful getc"); 
    TEST_OK(((c = Config_source_getc(cs)) == 'm'), "Successful getc"); 
    TEST_OK(((c = Config_source_getc(cs)) == 'i'), "Successful getc"); 
    TEST_OK(((c = Config_source_getc(cs)) == 't'), "Successful getc"); 

    Config_source_ungetc(cs, c);
    TEST_OK(((c = Config_source_getc(cs)) == 't'), "Successful ungetc"); 
    TEST_OK(((c = Config_source_getc(cs)) == ' '), "Successful getc"); 

    Config_source_destroy(cs);
    free(test_file);

    TEST_COMPLETE;
}
