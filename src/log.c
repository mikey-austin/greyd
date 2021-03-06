/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>
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
 * @file   log.c
 * @brief  Logging interface implementation.
 * @author Mikey Austin
 * @date   2014-2020
 */

#include "constants.h"
#include "greyd_config.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

static short Log_debug = 0;
static short Log_syslog = 0;
static const char* Log_ident = NULL;
static FILE* log_file = NULL;
static struct flock lock;

extern void Log_reinit(Config_T config)
{
    // Lock the entire file.
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    lock.l_start = 0;
    lock.l_pid = 0;

    char* path = NULL;
    if ((path = Config_get_str(config, "log_to_file", NULL, NULL)) != NULL) {
        log_file = fopen(path, "a");
    }
}

extern void
Log_setup(Config_T config, const char* prog_name)
{
    Log_ident = prog_name;
    Log_debug = Config_get_int(config, "debug", NULL, 0);

    if (Config_get_int(config, "syslog_enable", NULL, 1)) {
        Log_syslog = 1;
        openlog(Log_ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    }

    Log_reinit(config);
}

static void
write_to(FILE* f, const char* msg, va_list args)
{
    // Lock the file for writing.
    lock.l_type = F_WRLCK;
    if (fcntl(fileno(f), F_SETLK, &lock) == -1) {
        return;
    }

    fprintf(f, "%s[%d]: ", Log_ident, getpid());
    vfprintf(f, msg, args);
    fprintf(f, "\n");
    fflush(f);

    // Release the lock.
    lock.l_type = F_UNLCK;
    if (fcntl(fileno(f), F_SETLK, &lock) == -1) {
        return;
    }
}

extern void
Log_write(int severity, const char* msg, va_list args)
{
    va_list syslog_args, file_args;

    if (Log_debug || (!Log_debug && severity < LOG_DEBUG)) {
        if (Log_syslog) {
            va_copy(syslog_args, args);
            vsyslog(severity, msg, syslog_args);
            va_end(syslog_args);
        }

        if (log_file != NULL) {
            va_copy(file_args, args);
            write_to(log_file, msg, file_args);
        }

        write_to(stderr, msg, args);
    }
}
