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
    Greylister_T greylister;
    Config_T c;
    int ret, i = 0;
    char *conf =
        "low_prio_mx_ip = \"192.179.21.3\"\n"
        "sync = 0\n"
        "section grey {\n"
        "  traplist_name    = \"test traplist\",\n"
        "  traplist_message = \"you have been trapped\",\n"
        "  grey_expiry      = 3600\n"
        "}\n"
        "section database {\n"
        "  driver = \"../modules/bdb.so\",\n"
        "  path   = \"/tmp/greyd_test.db\"\n"
        "}";

    TEST_START(4);

    c = Config_create();
    ls = Lexer_source_create_from_str(conf, strlen(conf));
    l = Config_lexer_create(ls);
    cp = Config_parser_create(l);
    Config_parser_start(cp, c);

    greylister = Grey_setup(c);
    TEST_OK((greylister != NULL), "Greylister created successfully");
    TEST_OK(!strcmp(greylister->traplist_name, "test traplist"),
        "greylister name set correctly");
    TEST_OK(!strcmp(greylister->traplist_msg, "you have been trapped"),
        "greylister msg set correctly");

    Grey_finish(&greylister);
    TEST_OK((greylister == NULL), "Greylister destroyed successfully");

    Config_destroy(&c);
    Config_parser_destroy(&cp);

    TEST_COMPLETE;
}
