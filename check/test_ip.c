/**
 * @file   test_ip.c
 * @brief  Unit tests for the ip utility functions.
 * @author Mikey Austin
 * @date   2014
 */

#include "test.h"
#include <ip.h>
#include <list.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static u_int32_t stoi(const char *address);
static void cidr_destroy(void *cidr);

int
main(void)
{
    struct IP_cidr cidr;
    struct IP_addr a, m, b;
    u_int32_t start, end;
    char *s, *c;
    List_T cidrs;
    struct List_entry *entry;
    int i = 0;
    struct sockaddr_storage ss;

    TEST_START(23);

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

        TEST_OK((strcmp(s, c) == 0), "Range to CIDR ok");
        i++;
    }

    List_destroy(&cidrs);

    /* Test IPv4 matching. */
    a.v4.s_addr = 0xC0A80C01; /* 192.168.12.1 */
    m.v4.s_addr = 0xffffff00; /* 255.255.255.0 */
    b.v4.s_addr = 0xC0A80C18; /* 192.168.12.24 */

    a.addr32[0] = a.addr32[0] & m.addr32[0];
    TEST_OK((IP_match_addr(&a, &m, &b, AF_INET) > 0), "IPv4 matches as expected");

    b.v4.s_addr = 0xC0A80D18; /* 192.168.13.24 */
    TEST_OK((IP_match_addr(&a, &m, &b, AF_INET) == 0), "IPv4 mismatch as expected");

    /* Test IPv6 matching. */
    a.addr32[0] = 0xfe800000; /* FE80::0202:B3FF:FE1E:2201 */
    a.addr32[1] = 0x00000000;
    a.addr32[2] = 0x0202b3ff;
    a.addr32[3] = 0xfe1e2201;

    m.addr32[0] = 0xffffffff; /* CIDR /120 */
    m.addr32[1] = 0xffffffff;
    m.addr32[2] = 0xffffffff;
    m.addr32[3] = 0xffffff00;

    b.addr32[0] = 0xfe800000; /* FE80::0202:B3FF:FE1E:2205 */
    b.addr32[1] = 0x00000000;
    b.addr32[2] = 0x0202b3ff;
    b.addr32[3] = 0xfe1e2205;

    for(i = 0; i < 4; i++)
        a.addr32[i] = a.addr32[i] & m.addr32[i];
    TEST_OK((IP_match_addr(&a, &m, &b, AF_INET6) > 0), "IPv6 matches as expected");

    b.addr32[3] = 0x11223344;
    TEST_OK((IP_match_addr(&a, &m, &b, AF_INET6) == 0), "IPv6 mismatch as expected");

    i = IP_check_addr("123224jhjj");
    TEST_OK((i == -1), "match fail as expected");

    i = IP_check_addr("1.2.3.4");
    TEST_OK((i == AF_INET), "match as expected");

    i = IP_check_addr("1.2.3");
    TEST_OK((i == AF_INET), "match as expected");

    i = IP_check_addr("2001::fad3:1");
    TEST_OK((i == AF_INET6), "match as expected");

    i = IP_check_addr("fe80::2c0:8cff:fe01:2345");
    TEST_OK((i == AF_INET6), "match as expected");

    /* Test the sockaddr_storage to struct IP_addr conversions. */
    ((struct sockaddr_in *) &ss)->sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.12.1", &((struct sockaddr_in *) &ss)->sin_addr);
    i = IP_sockaddr_to_addr(&ss, &a);
    TEST_OK(i == AF_INET, "af conversion ok");
    TEST_OK(a.v4.s_addr == ntohl(0xC0A80C01), "v4 addr conversion ok");

    ((struct sockaddr_in6 *) &ss)->sin6_family = AF_INET6;
    inet_pton(AF_INET6, "FE80::0202:B3FF:FE1E:2205", &((struct sockaddr_in6 *) &ss)->sin6_addr);
    i = IP_sockaddr_to_addr(&ss, &a);
    TEST_OK(i == AF_INET6, "af conversion ok");
    TEST_OK(a.addr32[0] == ntohl(0xfe800000)
            && a.addr32[1] == ntohl(0x00000000)
            && a.addr32[2] == ntohl(0x0202b3ff)
            && a.addr32[3] == ntohl(0xfe1e2205), "v6 addr conversion ok");

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
