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
 * @file   con.h
 * @brief  Defines the connection management interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CON_DEFINED
#define CON_DEFINED

#include "blacklist.h"
#include "grey.h"
#include "greyd.h"
#include "greyd_config.h"
#include "ip.h"
#include "list.h"

#define CON_BL_SUMMARY_SIZE 80
#define CON_BL_SUMMARY_ETC " ..."
#define CON_BUF_SIZE 8192
#define CON_DEFAULT_MAX 800
#define CON_REMOTE_END_SIZE 5
#define CON_GREY_STUTTER 10
#define CON_STUTTER 1
#define CON_OUT_BUF_SIZE 8192
#define CON_CLIENT_TOLERENCE 5
#define CON_ERROR_CODE "450"
#define CON_MAX_BAD_CMD 20

/**
 * State machine connection states.
 */
#define CON_STATE_BANNER_SENT 0
#define CON_STATE_HELO_IN 1
#define CON_STATE_HELO_OUT 2
#define CON_STATE_MAIL_IN 3
#define CON_STATE_MAIL_OUT 4
#define CON_STATE_RCPT_IN 5
#define CON_STATE_RCPT_OUT 6
#define CON_STATE_DATA_IN 50
#define CON_STATE_DATA_OUT 60
#define CON_STATE_MESSAGE 70
#define CON_STATE_REPLY 98
#define CON_STATE_CLOSE 99

/**
 * Main structure encapsulating the state of a single connection.
 */
struct Con {
    int fd;
    int state;
    int last_state;

    struct sockaddr_storage src;
    char src_addr[INET6_ADDRSTRLEN];
    char dst_addr[INET6_ADDRSTRLEN];
    char helo[GREY_MAX_MAIL];
    char mail[GREY_MAX_MAIL];
    char rcpt[GREY_MAX_MAIL];

    List_T blacklists; /* Blacklists containing this src address. */
    char* lists; /* Summary of associated blacklists. */

    /*
     * We will do stuttering by changing these to time_t's of
     * now + n, and only advancing when the time is in the past/now.
     */
    time_t r;
    time_t w;
    time_t s;

    char in_buf[CON_BUF_SIZE];
    char* in_p; /* This element's position in the struct is significant. */
    int in_remaining;

    /* Chars causing input termination. */
    char r_end_chars[CON_REMOTE_END_SIZE];

    char* out_buf;
    size_t out_size;
    char* out_p;
    int out_remaining;

    int data_lines;
    int data_body;
    int stutter;
    int bad_cmd;
    int seen_cr;
};

/**
 * Initialize a connection's internal state.
 */
extern void Con_init(struct Con* con, int fd, struct sockaddr_storage* src,
    struct Greyd_state* state);

/**
 * Cleanup a connection, to be re-initialized later.
 */
extern void Con_close(struct Con* con, struct Greyd_state* state);

/**
 * Advance a connection's SMTP state machine.
 */
extern void Con_next_state(struct Con* con, time_t* now,
    struct Greyd_state* state);

/**
 * Process any connection client input waiting to be
 * read on the connection's file descriptor, then
 * advance the state.
 */
extern void Con_handle_read(struct Con* con, time_t* now,
    struct Greyd_state* state);

/**
 * According to the current connection state, write the
 * contents of the connection's output buffer to the
 * configured file descriptor, then advance to the next
 * state.
 */
extern void Con_handle_write(struct Con* con, time_t* now,
    struct Greyd_state* state);

/**
 * Attempt to find and set the address the client originally
 * connected to.
 */
extern void Con_get_orig_dst(struct Con* con, struct Greyd_state* state);

/**
 * Return a suitably sized string summarizing the lists containing,
 * this connection's src address. If there are too many lists,
 * the string will be truncated with a trailing "..." appended.
 *
 * The returned string must be freed by the caller when it is no
 * longer needed.
 */
extern char* Con_summarize_lists(struct Con* con);

/**
 * Construct a reply consisting of each associated blacklist's
 * error string separated by a new line. If there are no associated
 * blacklists, return a generic reply.
 */
extern void Con_build_reply(struct Con* con, char* response_code);

/**
 * Prepare an error string and append to the supplied connection's
 * output buffer, from the specified offset. Special characters in the
 * error string will be substituted (eg %A, \\, \n, ...).
 *
 * @return Size of appended string.
 * @return -1 on error.
 */
extern int Con_append_error_string(struct Con* con, size_t off, char* fmt,
    char* response_code, int* last_line_cont);

/**
 * Increase the size of a connection's output buffer by a fixed
 * amount.
 *
 * @return Offset from the newly allocated buffer.
 * @return NULL on error.
 */
extern char* Con_grow_out_buf(struct Con* con, int off);

/**
 * Accept and initialize new connection in on fd. This funcion should be
 * run following a relevant call to accept().
 */
extern void Con_accept(int fd, struct sockaddr_storage* addr,
    struct Greyd_state* state);

#endif
