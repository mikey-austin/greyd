/**
 * @file   utils.h
 * @brief  Defines various system support utilities.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef UTILS_DEFINED
#define UTILS_DEFINED

#include <stdlib.h>

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

#endif
