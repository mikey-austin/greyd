/*
 * Copyright (c) 2016 Mikey Austin <mikey@greyd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file   test_plugin_framework.c
 * @brief  Unit tests for the plugin framework.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <plugin.h>
#include <greyd_config.h>
#include <config_parser.h>
#include <log.h>

#include <string.h>

/* Global used for testing loading/unloading. */
int Test_loaded = 0;
int Test_spamtrap = 0;
int Test_hook = 0;

int
main(int argc, char *argv[])
{
    Config_T c;
    Lexer_source_T ls;
    Lexer_T l;
    int result;
    Config_parser_T cp;
    char *conf =
        "debug=1\n"
        "section plugins {\n"
        "  enable = 1\n"
        "}\n"
        "plugin dummy {\n"
        "  driver = \"greyd_plugin_dummy.so\"\n"
        "}";

    c = Config_create();
    ls = Lexer_source_create_from_str(conf, strlen(conf));
    l = Config_lexer_create(ls);
    cp = Config_parser_create(l);
    Config_parser_start(cp, c);
    Log_setup(c, argv[0]);

    TEST_START(8);

    TEST_OK(Test_loaded == 0, "Initial state ok");
    TEST_OK(Test_hook == 0, "Initial hook state ok");
    TEST_OK(Test_spamtrap == 0, "Initial spamtrap state ok");
    Plugin_sys_init(c);
    TEST_OK(Test_loaded == 1, "Dummy plugin loaded");

    /* Test running of hook callbacks. */
    Plugin_run_callbacks(HOOK_NEW_ENTRY);
    TEST_OK(Test_hook == 1, "Hook callback ok");

    /* Test spamtraps. */
    result = Plugin_run_spamtraps(NULL);
    TEST_OK(Test_spamtrap == 1, "Spamtrap ok");
    TEST_OK(result == 0, "Spamtrap return ok");

    Config_destroy(&c);
    Config_parser_destroy(&cp);
    Plugin_sys_stop();
    TEST_OK(Test_loaded == 0, "Dummy plugin unloaded");

    TEST_COMPLETE;
}
