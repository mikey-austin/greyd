/**
 * @file   ip.h
 * @brief  Defines various networking support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef IP_DEFINED
#define IP_DEFINED

#include "list.h"

#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>

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

#endif
