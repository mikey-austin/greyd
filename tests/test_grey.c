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

static void write_non_grey(int, char *, char *, long, FILE *);
static void write_grey(char *, char *, char *, char *, char *, FILE *);
static void write_trap(char *source, char *ip, long expires, FILE *grey_out);
static void write_white(char *source, char *ip, long expires, FILE *grey_out);
static void add_spamtrap(char *trapaddr, Config_T config);

int
main()
{
    DB_handle_T db;
    DB_itr_T itr;
    struct DB_key key;
    struct DB_val val;
    struct Grey_tuple gt;
    struct Grey_data gd;
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

    TEST_START(14);

    c = Config_create();
    ls = Lexer_source_create_from_str(conf, strlen(conf));
    l = Config_lexer_create(ls);
    cp = Config_parser_create(l);
    Config_parser_start(cp, c);

    add_spamtrap("trap@test.com", c);

    greylister = Grey_setup(c);
    TEST_OK((greylister != NULL), "Greylister created successfully");
    TEST_OK(!strcmp(greylister->traplist_name, "test traplist"),
        "greylister name set correctly");
    TEST_OK(!strcmp(greylister->traplist_msg, "you have been trapped"),
        "greylister msg set correctly");
    TEST_OK(!strcmp(greylister->low_prio_mx_ip, "192.179.21.3"),
        "greylister low priority mx ip set correctly");

    pipe(grey);
    pipe(trap);

    grey_out = fdopen(grey[1], "w");
    grey_in  = fdopen(grey[0], "r");
    trap_out = fdopen(trap[1], "w");
    trap_in  = fdopen(trap[0], "r");

    greylister->startup  = time(NULL) - 120; /* Simulate a startup time in the past. */
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
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
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

    /* Write some grey entries. */
    write_grey("2.3.4.5", "1.2.3.4", "jackiemclean.net", "m@jackiemclean.net", "r@hotmail.com", grey_out);
    write_grey("2.3.1.5", "1.2.4.4", "jackiemclean.net", "m@jackiemclean.net", "r@hotmail.com", grey_out);
    write_grey("2.3.2.5", "1.2.2.4", "jackiemclean.net", "m@jackiemclean.net", "r@hotmail.com", grey_out);

    /* Write the duplicate entries. */
    write_grey("2.3.4.5", "1.2.3.4", "jackiemclean.net", "m@jackiemclean.net", "r@hotmail.com", grey_out);
    write_grey("2.3.1.5", "1.2.4.4", "jackiemclean.net", "m@jackiemclean.net", "r@hotmail.com", grey_out);
    write_grey("2.3.2.5", "1.2.2.4", "jackiemclean.net", "m@jackiemclean.net", "r@hotmail.com", grey_out);

    /* Write some white entries. */
    write_white("2.3.4.5", "4.3.2.1", time(NULL) + 3600, grey_out);
    write_white("2.3.4.6", "4.3.2.2", time(NULL) + 3600, grey_out);
    write_white("2.3.4.7", "4.3.2.3", time(NULL) + 3600, grey_out);

    /* Write duplicate white entries. */
    write_white("2.3.4.5", "4.3.2.1", time(NULL) + 3600, grey_out);
    write_white("2.3.4.6", "4.3.2.2", time(NULL) + 3600, grey_out);
    write_white("2.3.4.7", "4.3.2.3", time(NULL) + 3600, grey_out);

    /* Write some trap entries. */
    write_trap("3.2.4.5", "3.4.2.1", time(NULL) + 3600, grey_out);
    write_trap("3.2.4.6", "3.4.2.2", time(NULL) + 3600, grey_out);
    write_trap("3.2.4.7", "3.4.3.2", time(NULL) + 3600, grey_out);

    /* Write some duplicate trap entries. */
    write_trap("3.2.4.5", "3.4.2.1", time(NULL) + 3600, grey_out);
    write_trap("3.2.4.6", "3.4.2.2", time(NULL) + 3600, grey_out);
    write_trap("3.2.4.7", "3.4.3.2", time(NULL) + 3600, grey_out);

    /* Check spam trap address to test the trap checking for grey entries. */
    write_grey("2.3.2.5", "1.2.2.4", "jackiemclean.net", "m@jackiemclean.net", "trap@test.com", grey_out);
    write_grey("2.3.2.5", "1.2.2.4", "jackiemclean.net", "m@jackiemclean.net", "trap@test.com", grey_out);

    /* Trigger a hit to the low-priority MX server. */
    write_grey("192.179.21.3", "1.2.2.34", "jackiemclean.net", "m@jackiemclean.net",
               "notrap@test.com", grey_out);

    /* Forcing a parse error will kill the reader process. */
    fprintf(grey_out, "==\n");
    fclose(grey_out);
    close(grey[1]);
    waitpid(reader_pid, &ret, 0);

    /*
     * Test the overall database to ensue that the reader process correctly managed
     * the different types of entries.
     */
    int total_entries = 0, total_grey = 0, total_white = 0, total_trapped = 0, total_spamtrap = 0;
    int total_white_passed = 0, total_white_blocked = 0;
    int total_grey_passed = 0, total_grey_blocked = 0;

    db = DB_open(c, 0);
    itr = DB_get_itr(db);
    while(DB_itr_next(itr, &key, &val) != GREYDB_NOT_FOUND) {
        total_entries++;

        switch(key.type) {
        case DB_KEY_IP:
            if(val.data.gd.pcount == -1) {
                total_trapped++;
            }
            else {
                total_white++;
                total_white_passed += val.data.gd.pcount;
                total_white_blocked += val.data.gd.bcount;
            }
            break;

        case DB_KEY_MAIL:
            total_spamtrap++;
            break;

        case DB_KEY_TUPLE:
            total_grey++;
            total_grey_passed += val.data.gd.pcount;
            total_grey_blocked += val.data.gd.bcount;
            break;
        }
    }

    DB_close_itr(&itr);
    DB_close(&db);

    TEST_OK(total_entries == 12, "Total entries as expected");
    TEST_OK(total_white == 3, "Total white as expected");
    TEST_OK(total_grey == 3, "Total grey as expected");
    TEST_OK(total_trapped == 5, "Total trapped entries as expected");
    TEST_OK(total_spamtrap == 1, "Total spamtraps as expected");
    TEST_OK(total_white_passed == 3, "Total white passed as expected");
    TEST_OK(total_white_blocked == 0, "Total white blocked as expected");
    TEST_OK(total_grey_passed == 0, "Total grey passed as expected");
    TEST_OK(total_grey_blocked == 6, "Total grey blocked as expected");

cleanup:
    Grey_finish(&greylister);
    TEST_OK((greylister == NULL), "Greylister destroyed successfully");

    Config_destroy(&c);
    Config_parser_destroy(&cp);

    TEST_COMPLETE;
}

static void
add_spamtrap(char *trapaddr, Config_T config)
{
    DB_handle_T db;
    struct DB_key key;
    struct DB_val val;
    struct Grey_data gd;

    key.type = DB_KEY_MAIL;
    key.data.s = trapaddr;

    memset(&gd, 0, sizeof(gd));
    gd.first = time(NULL);
    gd.bcount = 1;
    gd.expire = 0;
    gd.pcount = -2;

    val.type = DB_VAL_GREY;
    val.data.gd = gd;

    db = DB_open(config, 0);
    DB_put(db, &key, &val);
    DB_close(&db);
}

static void
write_grey(char *dstip, char *ip, char *helo, char *from, char *to, FILE *grey_out)
{
    fprintf(grey_out,
            "type = %d\n"
            "dst_ip = \"%s\"\n"
            "ip = \"%s\"\n"
            "helo = \"%s\"\n"
            "from = \"%s\"\n"
            "to = \"%s\"\n"
            "%%\n", GREY_MSG_GREY, dstip, ip, helo, from, to);
    fflush(grey_out);
}

static void
write_non_grey(int type, char *source, char *ip, long expires, FILE *grey_out)
{
    fprintf(grey_out,
            "type = %d\n"
            "ip = \"%s\"\n"
            "source = \"%s\"\n"
            "expires = \"%ld\"\n"
            "%%\n", type, ip, source, expires);
    fflush(grey_out);
}

static void
write_white(char *source, char *ip, long expires, FILE *grey_out)
{
    write_non_grey(GREY_MSG_WHITE, source, ip, expires, grey_out);
}

static void
write_trap(char *source, char *ip, long expires, FILE *grey_out)
{
    write_non_grey(GREY_MSG_TRAP, source, ip, expires, grey_out);
}
