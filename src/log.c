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
 * @date   2014
 */

#include "constants.h"
#include "greyd_config.h"

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

static short Log_debug = 0;
static short Log_syslog = 0;
static const char* Log_ident = NULL;

extern void
Log_setup(Config_T config, const char* prog_name)
{
    Log_ident = prog_name;
    Log_debug = Config_get_int(config, "debug", NULL, 0);

    if (Config_get_int(config, "syslog_enable", NULL, 1)) {
        Log_syslog = 1;
        openlog(Log_ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    }
}

extern void
Log_write(int severity, const char* msg, va_list args)
{
    va_list syslog_args;

    if (Log_debug || (!Log_debug && severity < LOG_DEBUG)) {
        if (Log_syslog) {
            va_copy(syslog_args, args);
            vsyslog(severity, msg, syslog_args);
            va_end(syslog_args);
        }

        /* Always write to console. */
        printf("%s[%d]: ", Log_ident, getpid());
        vprintf(msg, args);
        printf("\n");
    }
}
