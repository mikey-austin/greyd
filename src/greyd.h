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
 * @file   greyd.h
 * @brief  Defines greyd support constants and definitions.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef GREYD_DEFINED
#define GREYD_DEFINED

#include <stdio.h>
#include <signal.h>
#include <libev/ev.h>

#include "sync.h"
#include "firewall.h"
#include "hash.h"

/**
 * Structure to encapsulate the state of the main
 * greyd process.
 */
struct Greyd_state {
    Config_T config;
    int slow_until;

    volatile sig_atomic_t shutdown;

    struct ev_loop *loop;
    ev_io *cfg_watcher;

    int max_files;
    int max_cons;
    int max_black;
    int clients;
    int black_clients;

    pid_t fw_pid;
    FILE *grey_out;
    FILE *fw_out;
    FILE *fw_in;

    Hash_T blacklists;
    Sync_engine_T syncer;
};

/**
 * Process configuration input, and add resulting blacklist to the
 * state's list.
 */
extern void Greyd_process_config(int fd, struct Greyd_state *state);

/**
 * Send blacklist configuration to the specified file descriptor.
 */
extern void Greyd_send_config(FILE *out, char *bl_name, char *bl_msg, List_T ips);

/**
 * Process a request for the firewall process.
 */
extern void Greyd_process_fw_message(Config_T message, FW_handle_T fw_handle, FILE *out);

/**
 * Start the firewall management process.
 */
extern int Greyd_start_fw_child(struct Greyd_state *state, int in_fd, int nat_in_fd, int out_fd);

#endif
