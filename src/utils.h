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
 * @file   utils.h
 * @brief  Defines various system support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef UTILS_DEFINED
#define UTILS_DEFINED

#include <stdlib.h>

#ifndef MAX
#  define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#endif

/**
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left).  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occurred.
 */
extern size_t sstrncat(char *dst, const char *src, size_t dsize);

/**
 * Copy src to string dst of size dsize.  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 */
extern size_t sstrncpy(char *dst, const char *src, size_t dsize);

/**
 * Take a NUL-terminated email address, remove any '<' & '>' characters,
 * and ensure that it is lower case.
 */
extern void normalize_email_addr(const char *addr, char *buf, int buf_size);

#endif
