/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>
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
 * @file   ip.h
 * @brief  Defines various networking support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef IP_DEFINED
#define IP_DEFINED

#include "list.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define IP_MAX_MASKBITS    128
#define IP_MAX_MASKBITS_V4 32

/**
 * Structure to house a single IPv4 CIDR network.
 */
struct IP_cidr {
    u_int32_t addr;
    u_int8_t  bits;
};

/**
 * Structure to house an IPv4 or IPv6 address (ie 128 bits worth).
 */
struct IP_addr {
	union {
		struct in_addr	v4;
		struct in6_addr	v6;
		u_int8_t		addr8[16];
		u_int16_t		addr16[8];
		u_int32_t		addr32[4];
	} _addr;
#define v4	    _addr.v4
#define v6	    _addr.v6
#define addr8	_addr.addr8
#define addr16	_addr.addr16
#define addr32	_addr.addr32
};

/**
 * Convert the supplied cidr into it's corresponding range.
 */
extern void IP_cidr_to_range(struct IP_cidr *cidr, u_int32_t *start,
                             u_int32_t *end);

/**
 * Decompose the supplied range into a series of CIDRs, and populate the
 * supplied list. The number of CIDR blocks created is returned.
 */
extern int IP_range_to_cidr_list(List_T cidrs, u_int32_t start,
                                 u_int32_t end);


/**
 * Return a human-readable string representation of the CIDR block.
 *
 * The returned string must be cleaned up manually using free.
 */
extern char *IP_cidr_to_str(const struct IP_cidr *cidr);

/**
 * Return 1 if the addresses a (with mask m) matches address b
 * otherwise return 0. It is assumed that address a has been
 * pre-masked out, we only need to mask b.
 */
extern int IP_match_addr(struct IP_addr *a, struct IP_addr *m,
                         struct IP_addr *b, sa_family_t af);

/**
 * Check if a supplied NULL-terminated string is a valid
 * IPv4 or IPv6 address.
 *
 * @return AF_INET if the string is a valid IPv4 address.
 * @return AF_INET6 if the string is a valid IPv4 address.
 * @return -1 if the string is an invalid address.
 */
extern short IP_check_addr(const char *addr);

/**
 * Convert a sockaddr_storage address (from any address family)
 * into an internal IP_addr address format.
 *
 * @return The address family on success
 * @return -1 on failure
 */
extern int IP_sockaddr_to_addr(struct sockaddr_storage *ss,
                               struct IP_addr *addr);

#endif
