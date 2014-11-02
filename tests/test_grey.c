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
    DB_handle_T db;
    DB_itr_T itr;
    Lexer_source_T ls;
    Lexer_T l;
    Config_parser_T cp;
    Greylister_T greylister;
    Config_T c;
    int ret, i = 0, grey[2], trap[2];
    FILE *grey_in, *grey_out, *trap_in, *trap_out;
    pid_t reader_pid;
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
        "  path   = \"/tmp/greyd_test_grey.db\"\n"
        "}";

    /* Empty existing database file. */
    ret = unlink("/tmp/greyd_test_grey.db");
    if(ret < 0 && errno != ENOENT) {
        printf("Error unlinking test Berkeley DB: %s\n", strerror(errno));
    }

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

    pipe(grey);
    pipe(trap);

    grey_out = fdopen(grey[1], "w");
    grey_in  = fdopen(grey[0], "r");
    trap_out = fdopen(trap[1], "w");
    trap_in  = fdopen(trap[0], "r");

    greylister->startup  = time(NULL);
    greylister->grey_in  = grey_in;
    greylister->trap_out = trap_out;

    /* Start the grey reader child process. */
    switch(reader_pid = fork()) {
    case -1:
        goto cleanup;
        break;

    case 0:
        /* In child. */
        Grey_start_reader(greylister);
        fclose(trap_in);
        fclose(grey_out);
        close(trap[0]);
        close(grey[1]);
        exit(0);
        break;
    }

    /* In parent. */
    fclose(trap_out);
    fclose(grey_in);
    close(trap[1]);
    close(grey[0]);
    greylister->trap_out = NULL;
    greylister->grey_in = NULL;

    /* Send the reader entries over the pipe. */
    fprintf(grey_out,
            "type = %d\n"
            "dst_ip = \"2.3.4.5\"\n"
            "ip = \"1.2.3.4\"\n"
            "helo = \"jackiemclean.net\"\n"
            "from = \"mikey@jackiemclean.net\"\n"
            "to = \"recip@hotmail.com\"\n"
            "%%\n", GREY_MSG_GREY);
    fflush(grey_out);

    /* Forcing a parse error will kill the reader process. */
    fprintf(grey_out, "==\n");
    fclose(grey_out);
    close(grey[1]);
    waitpid(reader_pid, &ret, 0);

    /* Test the state of the database after the reader has died. */

cleanup:
    Grey_finish(&greylister);
    TEST_OK((greylister == NULL), "Greylister destroyed successfully");

    Config_destroy(&c);
    Config_parser_destroy(&cp);

    TEST_COMPLETE;
}
