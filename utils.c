/**
 * @file   utils.c
 * @brief  Support utilities for use throughout the system.
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/**
 * A "safe" strncpy, saves room for \0 at end of dest, and refuses to copy
 * more than "n" bytes.
 *
 * Taken from proftpd source.
 */
extern int
sstrncpy(char *dst, const char *src, size_t n)
{
#ifndef HAVE_STRLCPY
    register char *d = dst;
#endif /* HAVE_STRLCPY */
    int res = 0;

    if(dst == NULL) {
        errno = EINVAL;
        return -1;
    }

    if(src == NULL) {
        errno = EINVAL;
        return -1;
    }

    if(n == 0) {
        return 0;
    }

#ifdef HAVE_STRLCPY
    strlcpy(dst, src, n);

    /* We want the returned length to be the number of bytes copied as
     * requested by the caller, not the total length of the src string.
     */
    res = n;

#else
    if(src && *src) {
        for(; *src && n > 1; n--) {
            *d++ = *src++;
            res++;
        }
    }

    *d = '\0';
#endif /* HAVE_STRLCPY */

    return res;
}
