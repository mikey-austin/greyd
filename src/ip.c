/*
 * Copyright (c) 2014 Mikey Austin.  All rights reserved.
 * Copyright (c) 2003 Bob Beck.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   ip.c
 * @brief  Implements various IP support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#include "failures.h"
#include "ip.h"
#include "list.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

static u_int8_t max_diff(u_int32_t a, u_int32_t b);
static u_int8_t max_block(u_int32_t addr, u_int8_t bits);
static u_int32_t imask(u_int8_t b);

extern void
IP_cidr_to_range(struct IP_cidr *cidr, u_int32_t *start, u_int32_t *end)
{
    *start = cidr->addr;
    *end = cidr->addr + (1 << (32 - cidr->bits)) - 1;
}

extern int
IP_range_to_cidr_list(List_T cidrs, u_int32_t start, u_int32_t end)
{
    int nadded = 0;
    u_int8_t maxsize, diff;
    struct IP_cidr new;

    while(end >= start) {
        maxsize = max_block(start, 32);
        diff = max_diff(start, end);
        maxsize = (maxsize > diff ? maxsize : diff);

        new.addr = start;
        new.bits = maxsize;
        List_insert_after(cidrs, IP_cidr_to_str(&new));

        nadded++;
        start = start + (1 << (32 - maxsize));
    }

    return nadded;
}

extern int
IP_str_to_addr_mask(const char *address, struct IP_addr *n, struct IP_addr *m,
                    unsigned int *maskbits, short *af)
{
    int ret, i, j;
    char parsed[INET6_ADDRSTRLEN];
    unsigned int bits;

    memset(n, 0, sizeof(*n));
    memset(m, 0, sizeof(*m));

    ret = sscanf(address, "%39[^/]/%u", parsed, maskbits);
    if(ret != 2 || *maskbits == 0 || *maskbits > IP_MAX_MASKBITS)
        return -1;

    *af = (strchr(parsed, ':') != NULL ? AF_INET6 : AF_INET);
    if(*af == AF_INET && *maskbits > IP_MAX_MASKBITS_V4)
        return -1;

    if((ret = inet_pton(*af, parsed, n)) != 1)
        return -1;

    for(i = 0, j = 0; i < 4; i++)
        m->addr32[i] = 0;

    bits = *maskbits;
    while(bits >= 32) {
        m->addr32[j++] = 0xffffffff;
        bits -= 32;
    }

    for(i = 31; i > (31 - bits); --i)
        m->addr32[j] |= (1 << i);

    if(bits)
        m->addr32[j] = htonl(m->addr32[j]);

    /* Mask off address bits that won't ever be used. */
    for(i = 0; i < 4; i++)
        n->addr32[i] = n->addr32[i] & m->addr32[i];

    return 0;
}

extern char
*IP_cidr_to_str(const struct IP_cidr *cidr)
{
    char *str;
    struct in_addr in;

    if(cidr == NULL)
        return NULL;

    memset(&in, 0, sizeof(in));
    in.s_addr = htonl(cidr->addr);
    asprintf(&str, "%s/%u", inet_ntoa(in), cidr->bits);

    return str;
}

extern int
IP_match_addr(struct IP_addr *a, struct IP_addr *m, struct IP_addr *b,
              sa_family_t af)
{
    int match = 0;

    switch(af) {
    case AF_INET:
        if((a->addr32[0]) == (b->addr32[0] & m->addr32[0]))
            match++;
        break;

    case AF_INET6:
        if(((a->addr32[0]) == (b->addr32[0] & m->addr32[0]))
           && ((a->addr32[1]) == (b->addr32[1] & m->addr32[1]))
           && ((a->addr32[2]) == (b->addr32[2] & m->addr32[2]))
           && ((a->addr32[3]) == (b->addr32[3] & m->addr32[3])))
        {
            match++;
        }
        break;
    }

    return match;
}

extern short
IP_check_addr(const char *addr)
{
    sa_family_t sa;
    struct addrinfo hints, *res;

    sa = (strchr(addr, ':') != NULL ? AF_INET6 : AF_INET);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = sa;
    hints.ai_socktype = SOCK_DGRAM;  /* Dummy. */
    hints.ai_protocol = IPPROTO_UDP; /* Dummy. */
    hints.ai_flags = AI_NUMERICHOST;

    if(getaddrinfo(addr, NULL, &hints, &res) == 0) {
        free(res);
        return sa;
    }

    return -1;
}

extern int
IP_sockaddr_to_addr(struct sockaddr_storage *ss, struct IP_addr *addr)
{
    struct sockaddr *sa = (struct sockaddr *) ss;

    switch(sa->sa_family) {
    case AF_INET:
        addr->v4 = ((struct sockaddr_in *) sa)->sin_addr;
        break;

    case AF_INET6:
        addr->v6 = ((struct sockaddr_in6 *) sa)->sin6_addr;
        break;
    }

    return sa->sa_family;
}

static u_int8_t
max_diff(u_int32_t a, u_int32_t b)
{
    u_int8_t bits = 0;
    u_int32_t m;

    b++;
    while(bits < 32) {
        m = imask(bits);

        if((a & m) != (b & m))
            return (bits);

        bits++;
    }

    return bits;
}

static u_int8_t
max_block(u_int32_t addr, u_int8_t bits)
{
    u_int32_t m;

    while(bits > 0) {
        m = imask(bits - 1);

        if((addr & m) != addr)
            return bits;

        bits--;
    }

    return bits;
}

static u_int32_t
imask(u_int8_t b)
{
    if(b == 0)
        return 0;

    return (0xffffffff << (32 - b));
}
