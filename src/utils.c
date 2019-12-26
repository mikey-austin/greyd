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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "failures.h"
#include "utils.h"

#define PIDLEN 16

static int Pidfile_fd = -1;

extern size_t
sstrncat(char* dst, const char* src, size_t dsize)
{
    const size_t slen = strlen(src);
    const size_t dlen = strnlen(dst, dsize);

    if (dlen != dsize) {
        size_t count = slen;
        if (count > dsize - dlen - 1)
            count = dsize - dlen - 1;
        dst += dlen;
        memcpy(dst, src, count);
        dst[count] = '\0';
    }

    return slen + dlen;
}

extern size_t
sstrncpy(char* dst, const char* src, size_t dsize)
{
    const size_t slen = strlen(src);

    if (dsize != 0) {
        const size_t dlen = dsize > slen ? slen : dsize - 1;
        memcpy(dst, src, dlen);
        dst[dlen] = '\0';
    }

    return slen;
}

extern void
normalize_email_addr(const char* addr, char* buf, int buf_size)
{
    char* cp;

    if (*addr == '<')
        addr++;

    for (cp = buf; buf_size > 0 && *addr != '\0'; addr++) {
        /*
         * Copy valid characters into destination buffer. We disallow
         * backslashes and quote characters.
         */
        if (*addr != '\\' && *addr != '"' && *addr != '>' && *addr != '<')
            *cp++ = tolower((unsigned char)*addr);
    }
    buf[buf_size - 1] = '\0';
}

extern int
drop_privs(struct passwd* user)
{
#ifdef HAVE_SETRESGID
    if (setgroups(1, &user->pw_gid)
        || setresgid(user->pw_gid, user->pw_gid, user->pw_gid)
        || setresuid(user->pw_uid, user->pw_uid, user->pw_uid)) {
        return -1;
    }
#elif HAVE_SETREGID
    if (setgroups(1, &user->pw_gid)
        || setregid(user->pw_gid, user->pw_gid)
        || setreuid(user->pw_uid, user->pw_uid)) {
        return -1;
    }
#endif

    return 0;
}

extern int
write_pidfile(struct passwd* user, const char* path)
{
    char buf[PIDLEN + 1];
    int ret;
    struct flock lock;

    Pidfile_fd = open(path, (O_RDWR | O_CREAT), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (Pidfile_fd < 0) {
        return -1;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;

    if (fcntl(Pidfile_fd, F_SETLK, &lock) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            /* Another process has already locked the pidfile. */
            close(Pidfile_fd);
            ret = -2;
        } else {
            ret = -1;
        }
        close(Pidfile_fd);
        return ret;
    }

    ftruncate(Pidfile_fd, 0);
    snprintf(buf, PIDLEN, "%ld", (long)getpid());
    write(Pidfile_fd, buf, strlen(buf) + 1);

    if (fchown(Pidfile_fd, user->pw_uid, user->pw_gid) == -1) {
        close(Pidfile_fd);
        return -1;
    }

    /* We leave the pidfile fd open so that the lock remains. */
    return 0;
}

extern void
close_pidfile(const char* path, const char* chroot_dir)
{
    const char* chroot_pidfile;

    if (chroot_dir != NULL
        && (strlen(path) > strlen(chroot_dir))) {
        /* Try to remove the chroot path from the pidfile path. */
        chroot_pidfile = strstr(path, chroot_dir);
        if (chroot_pidfile == NULL)
            chroot_pidfile = path;
        else
            chroot_pidfile += strlen(chroot_dir);
    } else {
        chroot_pidfile = path;
    }

    if (Pidfile_fd > 0)
        close(Pidfile_fd);

    if (unlink(chroot_pidfile) != 0) {
        i_warning("could not unlink %s: %s",
            chroot_pidfile, strerror(errno));
    }
}
