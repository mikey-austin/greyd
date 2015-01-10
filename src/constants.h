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
 * @file   constants.h
 * @brief  Defines program-wide constants.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONSTANTS_DEFINED
#define CONSTANTS_DEFINED

#define GREYLISTING_ENABLED 1
#define IPV6_ENABLED        0
#define MAX_HOST_NAME       255
#define GREYD_PORT          8025
#define GREYD_SYNC_PORT     8025
#define GREYD_CFG_PORT      8026
#define GREYD_MAIN_USER     "greyd"
#define GREYD_DB_USER       "greydb"
#define GREYD_CHROOT        1
#define GREYD_CHROOT_DIR    "/var/empty"
#define GREYD_BACKLOG       10
#define MAX_FILES_THRESHOLD 200
#define MAX_TIME            400
#define POLL_TIMEOUT        1000 /* In milliseconds. */
#define GREYD_BANNER        "greyd IP-based SPAM blocker"
#define NUM_BLACKLISTS      10
#define TRACK_OUTBOUND       1

#endif
