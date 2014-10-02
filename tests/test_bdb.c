/**
 * @file   test_bdb.c
 * @brief  Unit tests for Berkeley DB driver.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../greydb.h"
#include "../config.h"
#include "../lexer.h"
#include "../config_parser.h"
#include "../config_lexer.h"

#include <string.h>

int
main()
{
    DB_handle_T db;
    Lexer_source_T ls;
    Lexer_T l;
    Config_parser_T cp;
    Config_T c;
    char *conf =
        "section database {\n"
        "  driver = \"../modules/bdb.so\",\n"
        "  path   = \"/tmp/greyd_test.db\"\n"
        "}";

    TEST_START(2);

    c = Config_create();
    ls = Lexer_source_create_from_str(conf, strlen(conf));
    l = Config_lexer_create(ls);
    cp = Config_parser_create(l);
    Config_parser_start(cp, c);

    db = DB_open(c);
    TEST_OK((db != NULL), "DB handle created successfully");
    TEST_OK((db->dbh != NULL), "BDB handle created successfully");

    DB_close(&db);
    Config_destroy(&c);
    Config_parser_destroy(&cp);

    TEST_COMPLETE;
}
