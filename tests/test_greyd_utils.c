/**
 * @file   test_greyd_utils.c
 * @brief  Unit tests for greyd utils.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <greyd.h>
#include <blacklist.h>
#include <hash.h>
#include <list.h>

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>

static void destroy_blacklist(struct Hash_entry *entry);

int
main()
{
    int com[2];
    FILE *out;
    List_T ips, ips2;
    Blacklist_T bl, bl2;
    struct Greyd_state state;

    TEST_START(9);

    pipe(com);
    out = fdopen(com[1], "w");

    ips = List_create(NULL);
    List_insert_after(ips, "10.9.9.9/12");
    List_insert_after(ips, "2.2.2.2/12");
    state.blacklists = Hash_create(10, destroy_blacklist);

    ips2 = List_create(NULL);
    List_insert_after(ips2, "192.168.1.1/24");

    Greyd_send_config(out, "test_bl", "you are blacklisted", ips);
    Greyd_process_config(com[0], &state);

    bl = Hash_get(state.blacklists, "test_bl");
    TEST_OK(bl != NULL, "blacklist fetched correctly");
    TEST_OK(!strcmp(bl->name, "test_bl"), "blacklist name ok");
    TEST_OK(!strcmp(bl->message, "you are blacklisted"), "blacklist msg ok");
    TEST_OK(bl->count == 2, "blacklist entries count ok");

    Greyd_send_config(out, "test_bl", "you 2 are blacklisted", ips2);
    Greyd_process_config(com[0], &state);

    bl2 = Hash_get(state.blacklists, "test_bl");
    TEST_OK(bl2 != NULL, "blacklist fetched correctly");
    TEST_OK(bl != bl2, "blacklist overwritten ok");
    TEST_OK(!strcmp(bl2->name, "test_bl"), "blacklist name ok");
    TEST_OK(!strcmp(bl2->message, "you 2 are blacklisted"), "blacklist msg ok");
    TEST_OK(bl2->count == 1, "blacklist entries count ok");

    List_destroy(&ips);
    List_destroy(&ips2);
    Hash_destroy(&state.blacklists);

    TEST_COMPLETE;
}

static void
destroy_blacklist(struct Hash_entry *entry)
{
    if(entry && entry->v) {
        Blacklist_destroy((Blacklist_T *) &entry->v);
    }
}
