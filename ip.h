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

/**
 * Structure to house a single CIDR network.
 */
struct IP_cidr {
    u_int32_t addr;
    u_int8_t  bits;
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
