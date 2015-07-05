/*
 * Copyright (c) 2015 Mikey Austin <mikey@greyd.org>
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
 * @file   events.h
 * @brief  Event management interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifdef HAVE_LIBEV_EV_H
#  include <libev/ev.h>
#endif
#ifdef HAVE_EV_H
#  include <ev.h>
#endif

#include "con.h"
#include "greyd.h"

struct Events_con {
    ev_io io;
    struct Con con;
    struct Greyd_state *state;
};

/**
 * Callback for accepting connections on main sockets.
 */
extern void Events_accept_cb(EV_P_ ev_io *w, int revents);

/**
 * Callback for a single connection.
 */
extern void Events_con_cb(EV_P_ ev_io *w, int revents);

/**
 * Callback for the stuttering of connections which need to write.
 */
extern void Events_con_timer_cb(EV_P_ ev_timer *w, int revents);

/**
 * Callback to accept config connections.
 */
extern void Events_config_accept_cb(EV_P_ ev_io *w, int revents);

/**
 * Callback to handle an accepted config connection.
 */
extern void Events_config_cb(EV_P_ ev_io *w, int revents);

/**
 * Callback to handle trap config from the greylister.
 */
extern void Events_trap_cb(EV_P_ ev_io *w, int revents);

/**
 * Callback to handle sync connections.
 */
extern void Events_sync_cb(EV_P_ ev_io *w, int revents);

/**
 * Signal handler callback.
 */
extern void Events_signal_cb(EV_P_ ev_signal *w, int revents);
