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
 * @file   failures.c
 * @brief  Implements the failure handling interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "log.h"
#include "constants.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>

#define RETURN_CRIT 1
#define RETURN_ERR  2

extern void
i_critical(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_CRIT, msg, args);
    va_end(args);

    exit(RETURN_CRIT);
}

extern void
i_error(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_ERR, msg, args);
    va_end(args);

    exit(RETURN_ERR);
}

extern void
i_warning(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_WARNING, msg, args);
    va_end(args);
}

extern void
i_info(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_INFO, msg, args);
    va_end(args);
}

extern void
i_debug(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_DEBUG, msg, args);
    va_end(args);
}
