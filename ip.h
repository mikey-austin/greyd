/**
 * @file   ip.h
 * @brief  Defines various networking support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef IP_DEFINED
#define IP_DEFINED

#include <stdint.h>

/**
 * Structure to house a single CIDR network.
 */
struct IP_cidr {
    uint32_t address;
    int      bits;
};

extern IP_cidr_to_range(struct IP_cidr *cidr, uint32_t *start, uint32_t *end);

#endif
