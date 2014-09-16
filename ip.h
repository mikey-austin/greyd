/**
 * @file   ip.h
 * @brief  Defines various networking support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef IP_DEFINED
#define IP_DEFINED

#include <stdint.h>
#include <stdlib.h>

/**
 * Structure to house a single CIDR network.
 */
struct IP_cidr {
    u_int32_t addr;
    u_int8_t  bits;
};

extern IP_cidr_to_range(struct IP_cidr *cidr, u_int32_t *start, u_int32_t *end);

#endif
