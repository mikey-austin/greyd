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
    DB_handle_T db;
    DB_itr_T itr;
    Lexer_source_T ls;
    Lexer_T l;
    Config_parser_T cp;
    Config_T c;
    struct DB_key key1, key2;
    struct DB_val val1, val2;
    struct Grey_tuple gt;
    struct Grey_data gd, gd2;
    int ret, i = 0;
    char *conf =
        "section database {\n"
        "  driver = \"../modules/bdb.so\",\n"
        "  path   = \"/tmp/greyd_test.db\"\n"
        "}";

    TEST_START(47);

    c = Config_create();
    ls = Lexer_source_create_from_str(conf, strlen(conf));
    l = Config_lexer_create(ls);
    cp = Config_parser_create(l);
    Config_parser_start(cp, c);

    db = DB_open(c, GREYDB_RW);
    TEST_OK((db != NULL), "DB handle created successfully");
    TEST_OK((db->dbh != NULL), "BDB handle created successfully");

    memset(&gt, 0, sizeof(gt));
    memset(&val1, 0, sizeof(val1));

    /* Configure some test data. */
    gt.ip = "192.168.12.1";
    gt.helo = "gmail.com";
    gt.from = "test@gmail.com";
    gt.to = "test@hotmail.com";
    key1.type = DB_KEY_TUPLE;
    key1.data.gt = gt;

    gd.first = 1;
    gd.pass = 2;
    gd.expire = 3;
    gd.bcount = 4;
    gd.pcount = 5;
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

    /* Test deletion. */
    ret = DB_del(db, &key1);
    TEST_OK((ret == GREYDB_OK), "Deletion worked as expected");

    /* Test getting & deleting a non-existant entry. */
    ret = DB_get(db, &key1, &val2);
    TEST_OK((ret == GREYDB_NOT_FOUND), "Get failed expected");

    ret = DB_del(db, &key1);
    TEST_OK((ret == GREYDB_NOT_FOUND), "Deletion failed as expected");

    /* Insert multiple records to test iteration. */
    DB_put(db, &key1, &val1);

    /* val 2. */
    gt.ip = "10.10.10.10";
    gt.helo = "hotmail.com";
    gt.from = "test2@hotmail.com";
    gt.to = "test2@gmail.com";

    gd.first = 2;
    gd.pass = 4;
    gd.expire = 3;
    gd.bcount = 2;
    gd.pcount = 1;

    key1.data.gt = gt;
    val1.data.gd = gd;
    DB_put(db, &key1, &val1);

    /* val 3 */
    gt.ip = "10.200.200.200";
    gt.helo = "2hotmail.com";
    gt.from = "2test2@hotmail.com";
    gt.to = "2test2@gmail.com";

    gd.first = 3;
    gd.pass = 140;
    gd.expire = 130;
    gd.bcount = 120;
    gd.pcount = 110;

    key1.data.gt = gt;
    val1.data.gd = gd;
    DB_put(db, &key1, &val1);

    /* Iterate over the 3 key/value pairs. */
    itr = DB_get_itr(db);
    TEST_OK((itr != NULL), "Iterator created successfully");
    TEST_OK((itr->current == -1), "Iterator current index OK");

    /* Clear values. */
    memset(&gt, 0, sizeof(gt));
    memset(&gd, 0, sizeof(gd));
    memset(&key1, 0, sizeof(key1));
    memset(&val1, 0, sizeof(val1));

    while(DB_itr_next(itr, &key1, &val1) != GREYDB_NOT_FOUND) {
        gt = key1.data.gt;
        gd = val1.data.gd;

        switch(gd.first) {
        case 1:
            TEST_OK(!strcmp(gt.ip, "192.168.12.1"), "ip ok");
            TEST_OK(!strcmp(gt.helo, "gmail.com"), "helo ok");
            TEST_OK(!strcmp(gt.from, "test@gmail.com"), "from ok");
            TEST_OK(!strcmp(gt.to, "test@hotmail.com"), "to ok");

            TEST_OK((gd.first == 1), "type attr ok");
            TEST_OK((gd.pass == 2), "pass attr ok");
            TEST_OK((gd.expire == 3), "expire attr ok");
            TEST_OK((gd.bcount == 4), "bcount attr ok");
            TEST_OK((gd.pcount == 5), "pcount attr ok");
            break;

        case 2:
            gt = key1.data.gt;
            TEST_OK(!strcmp(gt.ip, "10.10.10.10"), "ip ok");
            TEST_OK(!strcmp(gt.helo, "hotmail.com"), "helo ok");
            TEST_OK(!strcmp(gt.from, "test2@hotmail.com"), "from ok");
            TEST_OK(!strcmp(gt.to, "test2@gmail.com"), "to ok");

            gd = val1.data.gd;
            TEST_OK((gd.first == 2), "type attr ok");
            TEST_OK((gd.pass == 4), "pass attr ok");
            TEST_OK((gd.expire == 3), "expire attr ok");
            TEST_OK((gd.bcount == 2), "bcount attr ok");
            TEST_OK((gd.pcount == 1), "pcount attr ok");
            break;

        case 3:
            gt = key1.data.gt;
            TEST_OK(!strcmp(gt.ip, "10.200.200.200"), "ip ok");
            TEST_OK(!strcmp(gt.helo, "2hotmail.com"), "helo ok");
            TEST_OK(!strcmp(gt.from, "2test2@hotmail.com"), "from ok");
            TEST_OK(!strcmp(gt.to, "2test2@gmail.com"), "to ok");

            gd = val1.data.gd;
            TEST_OK((gd.first == 3), "type attr ok");
            TEST_OK((gd.pass == 140), "pass attr ok");
            TEST_OK((gd.expire == 130), "expire attr ok");
            TEST_OK((gd.bcount == 120), "bcount attr ok");
            TEST_OK((gd.pcount == 110), "pcount attr ok");
            break;
        }

        TEST_OK((i == itr->current), "Iterator index OK");
        i++;
    }

    TEST_OK((itr->current == 2), "Iterations as expected");

    DB_close_itr(&itr);
    TEST_OK((itr == NULL), "Iterator close OK");

    DB_close(&db);
    Config_destroy(&c);
    Config_parser_destroy(&cp);

    TEST_COMPLETE;
}
