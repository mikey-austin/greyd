/**
 * @file   test_blacklist.c
 * @brief  Unit tests for the blacklist interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../blacklist.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

static u_int32_t stoi(const char *address);

int
main()
{
    Blacklist_T bl;
    u_int32_t a1, b1;

    TEST_START(12);

    bl = Blacklist_create("Test List", "You have been blacklisted");
    TEST_OK((bl != NULL), "Blacklist created successfully");
    TEST_OK((strcmp(bl->name, "Test List") == 0), "Name set successfully");
    TEST_OK((strcmp(bl->message, "You have been blacklisted") == 0),
            "Message set successfully");
    TEST_OK((bl->size == BLACKLIST_INIT_SIZE),
            "Correct entry list size ");
    TEST_OK((bl->count == 0), "Correctly initialized list of entries");

    a1 = stoi("192.168.1.0");
    b1 = stoi("192.168.1.100");

    Blacklist_add_range(bl, a1, b1, BL_TYPE_BLACK);
    TEST_OK((bl->count == 2), "Added 2 elements");

    TEST_OK((bl->entries[0].address == htonl(a1)), "First address is correct");
    TEST_OK((bl->entries[0].black == 1), "First address is black");
    TEST_OK((bl->entries[0].white == 0), "First address is not white");

    TEST_OK((bl->entries[1].address == htonl(b1)), "Second address is correct");
    TEST_OK((bl->entries[1].black == -1), "Second address is black");
    TEST_OK((bl->entries[1].white == 0), "Second address is not white");

    Blacklist_destroy(bl);

    TEST_COMPLETE;
}

static u_int32_t
stoi(const char *address)
{
    u_int32_t addr;
    inet_pton(AF_INET, address, &addr);

    return addr;
}
