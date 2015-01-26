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

#include <config.h>

#include <sys/param.h>
#include <sys/types.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>

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

extern int
drop_privs(struct passwd *user)
{
#ifdef HAVE_SETRESGID
    if(setgroups(1, &user->pw_gid)
       || setresgid(user->pw_gid, user->pw_gid, user->pw_gid)
       || setresuid(user->pw_uid, user->pw_uid, user->pw_uid))
    {
        return -1;
    }
#elif HAVE_SETREGID
    if(setgroups(1, &user->pw_gid)
       || setregid(user->pw_gid, user->pw_gid)
       || setreuid(user->pw_uid, user->pw_uid))
    {
        return -1;
    }
#endif

    return 0;
}

extern int
write_pidfile(struct passwd *user, const char *path)
{
    FILE *pidfile;
    int ret;

    if((ret = access(path, F_OK)) == 0) {
        return -2;
    }
    else if(ret == -1 && errno != ENOENT) {
        return -1;
    }

    if((pidfile = fopen(path, "w")) == NULL)
       return -1;
    fprintf(pidfile, "%u\n", getpid());
    fclose(pidfile);

    if(chown(path, user->pw_uid, user->pw_gid) == -1)
        return -1;

    return 0;
}
