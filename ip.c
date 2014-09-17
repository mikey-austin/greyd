/**
 * @file   ip.c
 * @brief  Implements various IP support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#include "ip.h"

extern void
IP_cidr_to_range(struct IP_cidr *cidr, u_int32_t *start, u_int32_t *end)
{
	*start = cidr->addr;
	*end = cidr->addr + (1 << (32 - cidr->bits)) - 1;
}
