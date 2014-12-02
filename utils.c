/**
 * @file   utils.c
 * @brief  Support utilities for use throughout the system.
 */

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

#include <stdlib.h>
#include <string.h>

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

size_t
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
