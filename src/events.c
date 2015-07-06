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
 * @file   events.c
 * @brief  Implement event callbacks.
 * @author Mikey Austin
 * @date   2014
 */

#include <string.h>
#include <errno.h>
#include <time.h>
#ifdef HAVE_LIBEV_EV_H
#  include <libev/ev.h>
#endif
#ifdef HAVE_EV_H
#  include <ev.h>
#endif

#include "failures.h"
#include "events.h"
#include "greyd.h"
#include "con.h"
#include "greyd_config.h"
#include "constants.h"
#include "grey.h"
#include "sync.h"
#include "list.h"

extern void
Events_accept_cb(EV_P_ ev_io *w, int revents)
{
    int accept_fd, i;
    struct Greyd_state *state = w->data;
    struct sockaddr_storage src;
    socklen_t src_len = sizeof(src);
    struct Events_con *cw = NULL;

    if(revents & EV_READ) {
        memset(&src, 0, sizeof(src));
        accept_fd = accept(w->fd, (struct sockaddr *) &src, &src_len);
        if(accept_fd == -1) {
            switch(errno) {
            case EINTR:
            case ECONNABORTED:
                break;

            case EMFILE:
            case ENFILE:
                state->slow_for = 1;
                ev_break(loop, EVBREAK_ALL);
                break;

            default:
                i_warning("con accept: %s", strerror(errno));
            }
        }
        else {
            /* Ensure we don't hit the configured fd limit. */
            if((state->clients + 1) >= state->max_cons) {
                close(accept_fd);
                state->slow_for = 0;
            }
            else {
                cw = calloc(1, sizeof(*cw));
                if(cw == NULL)
                    i_critical("calloc: %s", strerror(errno));
                cw->state = state;
                Con_init(&cw->con, accept_fd, &src, state);
                ev_io_init((ev_io *) cw, Events_con_cb, accept_fd, EV_READ | EV_WRITE);
                ev_io_start(state->loop, (ev_io *) cw);
                i_info("%s: connected (%d/%d)%s%s",
                       cw->con.src_addr, state->clients, state->black_clients,
                       (cw->con.lists == NULL ? "" : ", lists: "),
                       (cw->con.lists == NULL ? "" : cw->con.lists));
            }
        }
    }
    else {
        i_warning("main socket read error");
    }
}

extern void
Events_con_cb(EV_P_ ev_io *w, int revents)
{
    time_t now = time(NULL);
    struct Events_con *cw = (struct Events_con *) w;

    if(cw->con.fd == -1)
        goto cleanup;

    if(revents & EV_READ) {
        Con_handle_read(&cw->con, &now, cw->state);
    }
    else if(revents & EV_WRITE) {
        Con_handle_write(&cw->con, &now, cw->state);
    }
    else if(revents & EV_ERROR) {
        i_warning("ev con error");
        Con_close(&cw->con, cw->state);
        goto cleanup;
    }

    if(cw->con.fd == -1)
        goto cleanup;

    /* Setup the next operation on this connection. */
    ev_io_stop(cw->state->loop, w);
    if(cw->con.r) {
        if(cw->con.r + MAX_TIME <= now) {
            Con_close(&cw->con, cw->state);
            goto cleanup;
        }
        ev_io_set((ev_io *) cw, cw->io.fd, EV_READ);
        ev_io_start(cw->state->loop, (ev_io *) cw);
    }

    if(cw->con.w) {
        if(cw->con.w + MAX_TIME <= now) {
            Con_close(&cw->con, cw->state);
            goto cleanup;
        }

        if(cw->con.w <= now) {
            ev_io_set((ev_io *) cw, cw->io.fd, EV_WRITE);
            ev_io_start(cw->state->loop, (ev_io *) cw);
        }
        else {
            /* Leave stopped until we can write. */
            ev_timer *ct = malloc(sizeof(*ct));
            if(ct == NULL)
                i_critical("malloc: %s", strerror(errno));
            ct->data = cw;
            ev_timer_init(ct, Events_con_timer_cb, (cw->con.w - now), 0.);
            ev_timer_start(cw->state->loop, ct);
        }
    }

    return;

cleanup:
    ev_io_stop(cw->state->loop, w);
    List_destroy(&cw->con.blacklists);
    free(cw);
}

extern void
Events_con_timer_cb(EV_P_ ev_timer *w, int revents)
{
    struct Events_con *cw = w->data;

    ev_timer_stop(cw->state->loop, w);
    free(w);

    /* Start the connection IO watcher as we should be able to write now. */
    ev_io_set((ev_io *) cw, cw->io.fd, EV_WRITE);
    ev_io_start(cw->state->loop, (ev_io *) cw);
}

extern void
Events_config_accept_cb(EV_P_ ev_io *w, int revents)
{
    int accept_fd;
    struct Greyd_state *state = w->data;
    struct sockaddr_storage src;
    socklen_t src_len = sizeof(src);

    if(revents & EV_READ) {
        memset(&src, 0, sizeof(src));
        accept_fd = accept(w->fd, (struct sockaddr *) &src, &src_len);
        if(accept_fd == -1) {
            switch(errno) {
            case EINTR:
            case ECONNABORTED:
                break;

            case EMFILE:
            case ENFILE:
                state->slow_for = 1;
                ev_break(loop, EVBREAK_ALL);
                break;

            default:
                i_warning("config accept: %s", strerror(errno));
            }
        }
        else if(ntohs(((struct sockaddr_in *) &src)->sin_port) >= IPPORT_RESERVED) {
            /* Only accept config connections from privileged ports. */
            close(accept_fd);
            accept_fd = -1;
            state->slow_for = 0;
        }
        else {
            /*
             * Stop any further config connections. We only process one
             * config connection at a time.
             */
            ev_io_stop(state->loop, state->cfg_watcher);

            /* Setup the config watcher. */
            ev_io *cw = malloc(sizeof(*cw));
            if(cw == NULL)
                i_critical("malloc: %s", strerror(errno));
            cw->data = state;
            ev_io_init(cw, Events_config_cb, accept_fd, EV_READ);
            ev_io_start(state->loop, cw);
            return;
        }
    }
    else {
        i_warning("config socket read error");
    }
}

extern void
Events_config_cb(EV_P_ ev_io *w, int revents)
{
    struct Greyd_state *state = w->data;

    ev_io_stop(state->loop, w);
    if(revents & EV_READ) {
        Greyd_process_config(w->fd, state);
    }
    else {
        i_warning("config read error");
    }

    /* Cleanup this connection. */
    close(w->fd);
    free(w);

    /* Start listening for more config connections. */
    ev_io_start(state->loop, state->cfg_watcher);
}

extern void
Events_trap_cb(EV_P_ ev_io *w, int revents)
{
    struct Greyd_state *state = w->data;

    if(revents & EV_READ)
        Greyd_process_config(w->fd, state);
    else
        i_warning("trap read error");
}

extern void
Events_sync_cb(EV_P_ ev_io *w, int revents)
{
    struct Greyd_state *state = w->data;

    if(revents & EV_READ)
        Sync_recv(state->syncer, state->grey_out);
    else
        i_warning("sync read error");
}

extern void
Events_signal_cb(EV_P_ ev_signal *w, int revents)
{
    struct Greyd_state *state = w->data;
    state->shutdown = 1;
    ev_break(loop, EVBREAK_ALL);
}
