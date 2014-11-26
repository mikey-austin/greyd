/**
 * @file   test_con.c
 * @brief  Unit tests for connection management module.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../con.h"
#include "../greyd.h"
#include "../config.h"
#include "../lexer.h"
#include "../config_parser.h"
#include "../config_lexer.h"
#include "../grey.h"
#include "../list.h"
#include "../blacklist.h"

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

int
main()
{
    Lexer_source_T ls;
    Lexer_T l;
    Config_parser_T cp;
    Config_T c;
    Config_section_T section;
    struct Greyd_state gs;
    struct Con con;
    Blacklist_T bl1, bl2, bl3;
    struct sockaddr_storage src;
    int ret;
    char *conf =
        "hostname = \"greyd.org\"\n"
        "banner   = \"greyd IP-based SPAM blocker\"\n"
        "section grey {\n"
        "  enable           = 1,\n"
        "  traplist_name    = \"test traplist\",\n"
        "  traplist_message = \"you have been trapped\",\n"
        "  grey_expiry      = 3600,\n"
        "  stutter          = 15\n"
        "}\n"
        "section firewall {\n"
        "  driver = \"../modules/fw_dummy.so\"\n"
        "}\n"
        "section database {\n"
        "  driver = \"../modules/bdb.so\",\n"
        "  path   = \"/tmp/greyd_test_grey.db\"\n"
        "}";

    /* Empty existing database file. */
    ret = unlink("/tmp/greyd_test_grey.db");
    if(ret < 0 && errno != ENOENT) {
        printf("Error unlinking test Berkeley DB: %s\n", strerror(errno));
    }

    TEST_START(32);

    c = Config_create();
    ls = Lexer_source_create_from_str(conf, strlen(conf));
    l = Config_lexer_create(ls);
    cp = Config_parser_create(l);
    Config_parser_start(cp, c);

    /*
     * Set up the greyd state and blacklists.
     */
    memset(&gs, 0, sizeof(gs));
    gs.config = c;
    gs.max_cons = 4;
    gs.blacklists = List_create(NULL);

    bl1 = Blacklist_create("blacklist_1", "You (%A) are on blacklist 1");
    bl2 = Blacklist_create("blacklist_2", "You (%A) are on blacklist 2");
    bl3 = Blacklist_create("blacklist_3_with_an_enormously_big_long_long_epic_epicly_long_large_name",
                           "Your address %A\\nis on blacklist 3");

    List_insert_after(gs.blacklists, bl1);
    List_insert_after(gs.blacklists, bl2);
    List_insert_after(gs.blacklists, bl3);

    Blacklist_add(bl1, "10.10.10.1/32");
    Blacklist_add(bl1, "10.10.10.2/32");

    Blacklist_add(bl2, "10.10.10.1/32");
    Blacklist_add(bl2, "10.10.10.2/32");
    Blacklist_add(bl2, "2001::fad3:1/128");

    Blacklist_add(bl3, "10.10.10.2/32");
    Blacklist_add(bl3, "10.10.10.3/32");
    Blacklist_add(bl3, "2001::fad3:1/128");

    /*
     * Start testing the connection management.
     */
    memset(&src, 0, sizeof(src));
    ((struct sockaddr_in *) &src)->sin_family = AF_INET;
    inet_pton(AF_INET, "10.10.10.1", &((struct sockaddr_in *) &src)->sin_addr);

    memset(&con, 0, sizeof(con));
    Con_init(&con, 0, &src, &gs);

    TEST_OK(con.state == 0, "init state ok");
    TEST_OK(con.last_state == 0, "last state ok");
    TEST_OK(List_size(con.blacklists) == 2, "blacklist matches ok");
    TEST_OK(!strcmp(con.src_addr, "10.10.10.1"), "src addr ok");
    TEST_OK(con.out_buf != NULL, "out buf ok");
    TEST_OK(con.out_p == con.out_buf, "out buf pointer ok");
    TEST_OK(con.out_size == CON_OUT_BUF_SIZE, "out buf size ok");
    TEST_OK(!strcmp(con.lists, "blacklist_1 blacklist_2"),
            "list summary ok");

    /* The size of the banner. */
    TEST_OK(con.out_remaining == 75, "out buf remaining ok");

    TEST_OK(gs.clients == 1, "clients ok");
    TEST_OK(gs.black_clients == 1, "blacklisted clients ok");

    /*
     * Test the closing of a connection.
     */
    Con_close(&con, &gs);
    TEST_OK(List_size(con.blacklists) == 0, "blacklist empty ok");
    TEST_OK(con.out_buf == NULL, "out buf ok");
    TEST_OK(con.out_p == NULL, "out buf pointer ok");
    TEST_OK(con.out_size == 0, "out buf size ok");
    TEST_OK(con.lists == NULL, "lists ok");

    TEST_OK(gs.clients == 0, "clients ok");
    TEST_OK(gs.black_clients == 0, "blacklisted clients ok");

    /* Test recycling a connection. */
    memset(&src, 0, sizeof(src));
    ((struct sockaddr_in6 *) &src)->sin6_family = AF_INET6;
    inet_pton(AF_INET6, "2001::fad3:1", &((struct sockaddr_in6 *) &src)->sin6_addr);

    Con_init(&con, 0, &src, &gs);

    TEST_OK(con.state == 0, "init state ok");
    TEST_OK(con.last_state == 0, "last state ok");
    TEST_OK(List_size(con.blacklists) == 2, "blacklist matches ok");
    TEST_OK(!strcmp(con.src_addr, "2001::fad3:1"), "src addr ok");
    TEST_OK(con.out_buf != NULL, "out buf ok");
    TEST_OK(con.out_p == con.out_buf, "out buf pointer ok");
    TEST_OK(con.out_size == CON_OUT_BUF_SIZE, "out buf size ok");

    /*
     * As the 3rd blacklist's name is really long, the summarize lists
     * function should truncate with a "...".
     */
    TEST_OK(!strcmp(con.lists, "blacklist_2 ..."),
            "list summary ok");

    /* The size of the banner. */
    TEST_OK(con.out_remaining == 75, "out buf remaining ok");

    TEST_OK(gs.clients == 1, "clients ok");
    TEST_OK(gs.black_clients == 1, "blacklisted clients ok");

    /*
     * Test the rejection message building.
     */
    Con_build_reply(&con, "451");
    TEST_OK(!strcmp(con.out_p,
                    "451-You (2001::fad3:1) are on blacklist 2\n"
                    "451-Your address 2001::fad3:1\n"
                    "451 is on blacklist 3\n"),
            "Blacklisted error response ok");
    Con_close(&con, &gs);

    /* Test recycling a connection, which is not on a blacklist. */
    memset(&src, 0, sizeof(src));
    ((struct sockaddr_in6 *) &src)->sin6_family = AF_INET6;
    inet_pton(AF_INET6, "fa40::fad3:1", &((struct sockaddr_in6 *) &src)->sin6_addr);

    Con_init(&con, 0, &src, &gs);
    TEST_OK(List_size(con.blacklists) == 0, "not on blacklist ok");

    Con_build_reply(&con, "551");
    TEST_OK(!strcmp(con.out_p,
                    "551 Temporary failure, please try again later.\r\n"),
            "greylisted error response ok");
    Con_close(&con, &gs);

    /* Cleanup. */
    List_destroy(&con.blacklists);
    List_destroy(&gs.blacklists);
    Blacklist_destroy(&bl1);
    Blacklist_destroy(&bl2);
    Blacklist_destroy(&bl3);
    Config_destroy(&c);
    Config_parser_destroy(&cp);

    TEST_COMPLETE;
}
