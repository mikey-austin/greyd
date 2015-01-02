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
 * @file   failures.h
 * @brief  Defines program-wide functions to handle failures.
 * @author Mikey Austin
 * @date   2014
 *
 * All of the error handling functions below log/print their messages, and
 * may exit depending on their severity.
 */

#ifndef FAILURES_DEFINED
#define FAILURES_DEFINED

#include <stdarg.h>

/**
 * The critical info function will log it's message and exit.
 */
extern void i_critical(const char *msg, ...);

/**
 * The error info function will log it's message and exit.
 */
extern void i_error(const char *msg, ...);

/**
 * The warning info function will just log it's message.
 */
extern void i_warning(const char *msg, ...);

/**
 * The info function will just log it's message.
 */
extern void i_info(const char *msg, ...);

/**
 * The debug function will just log it's message.
 */
extern void i_debug(const char *msg, ...);

#endif
