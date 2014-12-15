/**
 * @file   test_config.c
 * @brief  Unit tests for config.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <config.h>
#include <config_parser.h>

#include <string.h>

#define SEC1 "section 1"

#define VAR1 "variable 1"
#define VAR2 "variable 2"

#define VAL1 "value 1"
#define VAL2 123

int
main()
{
    Config_T c, m;
    Config_section_T s1, s2, s;
    Config_value_T v;
    List_T list;
    int *count;

    TEST_START(31);

    c = Config_create();
    TEST_OK((c != NULL), "Config created successfully");

    s1 = Config_section_create(SEC1);
    Config_section_set_str(s1, VAR1, VAL1);
    Config_section_set_int(s1, VAR2, VAL2);

    Config_add_section(c, s1);
    s = Config_get_section(c, SEC1);
    TEST_OK((s != NULL), "Non-null section fetched as expected");
    TEST_OK((strcmp(s->name, SEC1) == 0), "Fetched section matches expected name");

    s = Config_get_section(c, "this section doesn't exist");
    TEST_OK((s == NULL), "Non-existant section fetched as expected");

    Config_destroy(&c);

    /*
     * Test the recursive config loading & parsing from a blank config.
     */
    c = Config_create();
    Config_load_file(c, "data/config_test1.conf");

    s1 = Config_get_section(c, "storage");
    TEST_OK((s1 != NULL), "Storage section parsed correctly");

    v = Config_section_get(s1, "storage_driver");
    TEST_OK((v && (strcmp(v->v.s, "MySQL") == 0)), "Section variable overridden correctly");

    v = Config_section_get(s1, "db_host");
    TEST_OK((v && (strcmp(v->v.s, "localhost") == 0)), "Section variable overridden correctly");

    v = Config_section_get(s1, "db_port");
    TEST_OK((v && (v->v.i == 3306)), "Section variable overridden correctly");

    v = Config_section_get(s1, "db_name");
    TEST_OK((v && (strcmp(v->v.s, "greyd") == 0)), "Section variable overridden correctly");

    s2 = Config_get_section(c, CONFIG_DEFAULT_SECTION);
    TEST_OK((s2 != NULL), "Storage section parsed correctly");

    v = Config_section_get(s2, "ip_address");
    TEST_OK((v && (strcmp(v->v.s, "1.2.3.4") == 0)), "Default section variable left alone correctly");

    v = Config_section_get(s2, "another_global");
    TEST_OK((v && (strcmp(v->v.s, "this is overwritten") == 0)), "Default section variable overridden correctly");

    v = Config_section_get(s2, "limit");
    TEST_OK((v && (v->v.i == 25)), "Default section variable overridden correctly");

    s1 = Config_get_section(c, "cache");
    TEST_OK((s1 != NULL), "Storage section in included config parsed correctly");

    v = Config_section_get(s1, "cache_driver");
    TEST_OK((v && (strcmp(v->v.s, "memcached") == 0)), "Section variable overridden correctly");

    v = Config_section_get(s1, "port");
    TEST_OK((v && (v->v.i == 11211)), "Section variable overridden correctly");

    v = Config_section_get(s1, "host");
    TEST_OK((v && (strcmp(v->v.s, "localhost") == 0)), "Section variable overridden correctly");

    /* Check the hashed included file counts. */
    count = (int *) Hash_get(c->processed_includes, "data/config_test1.conf");
    TEST_OK((count && (*count == 1)), "First config include file count as expected");

    count = (int *) Hash_get(c->processed_includes, "data/config_test2.conf");
    TEST_OK((count && (*count == 1)), "Second config include file count as expected");

    count = (int *) Hash_get(c->processed_includes, "data/config_test3.conf");
    TEST_OK((count && (*count == 1)), "Third config include file count as expected");

    TEST_OK((Queue_size(c->includes) == 0), "Include file to process queue is empty as expected");

    Config_set_str(c, "mystr1", NULL, "mystr value");
    Config_set_str(c, "mystr1", "mysection1", "mystr1 value");
    Config_set_int(c, "myint1", NULL, 4321);
    Config_set_int(c, "myint1", "mysection2", 1234);

    TEST_OK(!strcmp(Config_get_str(c, "mystr1", NULL, NULL), "mystr value"), "str set/get ok");
    TEST_OK(!strcmp(Config_get_str(c, "mystr1", "mysection1", NULL), "mystr1 value"), "str set/get ok");
    TEST_OK(Config_get_int(c, "myint1", NULL, 0) == 4321, "int set/get ok");
    TEST_OK(Config_get_int(c, "myint1", "mysection2", 0) == 1234, "int set/get ok");

    m = Config_create();
    Config_set_str(m, "mystr1", NULL, "overwritten");
    Config_set_str(m, "mystr1", "mysection6", "preserved");
    Config_set_int(m, "myint1", "mysection2", 4);
    Config_set_int(m, "myint86", NULL, 4);
    Config_merge(m, c);

    TEST_OK(!strcmp(Config_get_str(m, "mystr1", NULL, NULL), "mystr value"), "str overwritten ok");
    TEST_OK(!strcmp(Config_get_str(m, "mystr1", "mysection1", NULL), "mystr1 value"), "new str/section ok");
    TEST_OK(Config_get_int(m, "myint86", NULL, 0) == 4, "new int ok");
    TEST_OK(Config_get_int(m, "myint1", "mysection2", 0) == 1234, "int overwrite");

    Config_append_list_str(m, "newlist", "newsection", "my str var");
    Config_append_list_str(m, "newlist", "newsection", "another var");

    list = Config_get_list(m, "newlist", "newsection");
    TEST_OK((list != NULL), "list created successfully");
    TEST_OK((List_size(list) == 2), "list entries ok");

    Config_destroy(&c);
    Config_destroy(&m);

    TEST_COMPLETE;
}
