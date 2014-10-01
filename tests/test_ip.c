/**
 * @file   test_ip.c
 * @brief  Unit tests for the ip utility functions.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include "../ip.h"
#include "../list.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

static u_int32_t stoi(const char *address);
static void cidr_destroy(void *cidr);

int
main()
{
    struct IP_cidr cidr, *c;
    u_int32_t start, end;
    char *s, *s2;
    List_T cidrs;
    struct List_entry_T *entry;
    int i = 0;

    TEST_START(10);

    start = stoi("192.168.1.0");
    end = stoi("192.168.1.100");

    cidr.addr = ntohl(start);
    cidr.bits = 24;
    s = IP_cidr_to_str(&cidr);
    TEST_OK((strcmp(s, "192.168.1.0/24") == 0), "CIDR to str OK");
    free(s);

    cidr.addr = ntohl(end);
    cidr.bits = 8;
    s = IP_cidr_to_str(&cidr);
    TEST_OK((strcmp(s, "192.168.1.100/8") == 0), "CIDR to str OK");
    free(s);

    cidr.addr = ntohl(stoi("10.10.0.0"));
    cidr.bits = 17;
    IP_cidr_to_range(&cidr, &start, &end);
    TEST_OK((start == ntohl(stoi("10.10.0.0"))), "CIDR to range start OK");
    TEST_OK((end == ntohl(stoi("10.10.127.255"))), "CIDR to range end OK");

    cidrs = List_create(cidr_destroy);
    start = ntohl(stoi("192.168.0.1"));
    end = ntohl(stoi("192.168.0.25"));
    IP_range_to_cidr_list(cidrs, start, end);

    LIST_FOREACH(cidrs, entry) {
        c = List_entry_value(entry);
        switch(i) {
        case 0:
            s = "192.168.0.1/32";
            break;

        case 1:
            s = "192.168.0.2/31";
            break;

        case 2:
            s = "192.168.0.4/30";
            break;

        case 3:
            s = "192.168.0.8/29";
            break;

        case 4:
            s = "192.168.0.16/29";
            break;

        case 5:
            s = "192.168.0.24/31";
            break;
        }

        s2 = IP_cidr_to_str(c);
        TEST_OK((strcmp(s, s2) == 0), "Range to CIDR ok");
        free(s2);
        i++;
    }

    List_destroy(&cidrs);

    TEST_COMPLETE;
}

static u_int32_t
stoi(const char *address)
{
    u_int32_t addr;
    inet_pton(AF_INET, address, &addr);

    return addr;
}

static void
cidr_destroy(void *cidr)
{
    if(cidr) {
        free(cidr);
        cidr = NULL;
    }
}
