/**
 * @file   ip.c
 * @brief  Implements various IP support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#define _GNU_SOURCE

#include "failures.h"
#include "ip.h"
#include "list.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

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
	struct IP_cidr *new;

	while(end >= start) {
		maxsize = max_block(start, 32);
		diff = max_diff(start, end);
		maxsize = (maxsize > diff ? maxsize : diff);

        if((new = (struct IP_cidr *) malloc(sizeof(*new))) == NULL)
            I_CRIT("malloc failed");

		new->addr = start;
        new->bits = maxsize;
        List_insert_after(cidrs, (void *) new);

        nadded++;
		start = start + (1 << (32 - maxsize));
	}

	return nadded;
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
	int	match = 0;

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
