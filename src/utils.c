/*
 * Copyright (c) 1998, 2014 Todd C. Miller <Todd.Miller@courtesan.com>
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
 * @file   utils.c
 * @brief  Support utilities for use throughout the system.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"

extern size_t
sstrncat(char *dst, const char *src, size_t dsize)
{
    const size_t slen = strlen(src);
    const size_t dlen = strnlen(dst, dsize);

    if(dlen != dsize) {
        size_t count = slen;
        if(count > dsize - dlen - 1)
            count = dsize - dlen - 1;
        dst += dlen;
        memcpy(dst, src, count);
        dst[count] = '\0';
    }

    return slen + dlen;
}

extern size_t
sstrncpy(char *dst, const char *src, size_t dsize)
{
    const size_t slen = strlen(src);

    if(dsize != 0) {
        const size_t dlen = dsize > slen ? slen : dsize - 1;
        memcpy(dst, src, dlen);
        dst[dlen] = '\0';
    }

    return slen;
}

extern void
normalize_email_addr(const char *addr, char *buf, int buf_size)
{
    char *cp;

    if(*addr == '<')
        addr++;

    sstrncpy(buf, addr, buf_size);
    cp = strrchr(buf, '>');
    if(cp != NULL && *(cp + 1) == '\0')
        *cp = '\0';

    cp = buf;
    while(*cp != '\0') {
        *cp = tolower((unsigned char) *cp);
        cp++;
    }
}
