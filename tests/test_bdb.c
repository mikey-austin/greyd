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
#include "../grey.h"

#include <string.h>

int
main()
{
    DB_handle_T db = NULL;
    Lexer_source_T ls;
    Lexer_T l;
    Config_parser_T cp;
    Config_T c;
    struct DB_key key1, key2;
    struct DB_val val1, val2;
    struct Grey_tuple gt;
    struct Grey_data gd, gd2;
    int ret;
    char *conf =
        "section database {\n"
        "  driver = \"../modules/bdb.so\",\n"
        "  path   = \"/tmp/greyd_test.db\"\n"
        "}";
    

    TEST_START(10);

    c = Config_create();
    ls = Lexer_source_create_from_str(conf, strlen(conf));
    l = Config_lexer_create(ls);
    cp = Config_parser_create(l);
    Config_parser_start(cp, c);

    db = DB_open(c);
    TEST_OK((db != NULL), "DB handle created successfully");
    TEST_OK((db->dbh != NULL), "BDB handle created successfully");

    memset(&key1, 0, sizeof(key1));
    memset(&key2, 0, sizeof(key2));
    memset(&val1, 0, sizeof(val1));
    memset(&val2, 0, sizeof(val2));
    memset(&gd, 0, sizeof(gd));
    memset(&gd2, 0, sizeof(gd2));
    memset(&gt, 0, sizeof(gt));

    /* Configure some test data. */
    strcpy(gt.ip, "192.168.12.1");
    strcpy(gt.helo, "gmail.com");
    strcpy(gt.from, "test@gmail.com");
    strcpy(gt.to, "test@hotmail.com");

    gd.first = 1;
    gd.pass = 2;
    gd.expire = 3;
    gd.bcount = 4;
    gd.pcount = 5;

    key1.type = DB_KEY_TUPLE;
    key1.data.gt = gt;

    val1.type = DB_VAL_GREY;
    val1.data.gd = gd;

    ret = DB_put(db, &key1, &val1);
    TEST_OK((ret == GREYDB_OK), "Put successful");

    ret = DB_get(db, &key1, &val2);
    TEST_OK((ret == GREYDB_FOUND), "Get successful");
    TEST_OK((val2.type == DB_VAL_GREY), "type ok");

    gd2 = val2.data.gd;
    TEST_OK((gd2.first == 1), "type attr ok");
    TEST_OK((gd2.pass == 2), "pass attr ok");
    TEST_OK((gd2.expire == 3), "expire attr ok");
    TEST_OK((gd2.bcount == 4), "bcount attr ok");
    TEST_OK((gd2.pcount == 5), "pcount attr ok");

    DB_close(&db);
    Config_destroy(&c);
    Config_parser_destroy(&cp);

    TEST_COMPLETE;
}
