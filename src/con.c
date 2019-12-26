/*
 * Copyright (c) 2014 Mikey Austin.  All rights reserved.
 * Copyright (c) 2002-2007 Bob Beck.  All rights reserved.
 * Copyright (c) 2002 Theo de Raadt.  All rights reserved.
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
 * @file   con.c
 * @brief  Implements the connection management interface.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "blacklist.h"
#include "con.h"
#include "config_parser.h"
#include "constants.h"
#include "failures.h"
#include "grey.h"
#include "hash.h"
#include "ip.h"
#include "list.h"
#include "utils.h"

#define DNAT_LOOKUP_TIMEOUT 1000 /* In ms. */

static int match(const char*, const char*);
static void get_helo(char*, size_t, char*);
static void set_log(char*, size_t, char*);
static void destroy_blacklist(void*);

extern void
Con_init(struct Con* con, int fd, struct sockaddr_storage* src,
    struct Greyd_state* state)
{
    time_t now;
    short greylist, grey_stutter;
    int ret;
    char *human_time, *bl_name;
    struct List_entry* entry;
    Blacklist_T blacklist = NULL, con_blacklist;
    List_T bl_names;
    struct IP_addr ipaddr;

    /* Free resources before zeroing out this entry. */
    if (con->out_buf != NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_size = 0;
    }

    if (con->blacklists != NULL)
        List_destroy(&con->blacklists);

    if (con->lists != NULL) {
        free(con->lists);
        con->lists = NULL;
    }

    memset(con, 0, sizeof *con);

    /* Start initializing the connection. */
    if (Con_grow_out_buf(con, 0) == NULL)
        i_critical("could not grow connection out buf");

    con->blacklists = List_create(destroy_blacklist);
    con->fd = fd;

    greylist = Config_get_int(state->config, "enable", "grey", GREYLISTING_ENABLED);
    grey_stutter = Config_get_int(state->config, "stutter", "grey", CON_GREY_STUTTER);
    con->stutter = (greylist && !grey_stutter && List_size(con->blacklists) == 0)
        ? 0
        : Config_get_int(state->config, "stutter", NULL, CON_STUTTER);

    memcpy(&(con->src), src, sizeof(*src));
    ret = getnameinfo((struct sockaddr*)src, IP_SOCKADDR_LEN(((struct sockaddr*)src)),
        con->src_addr, sizeof(con->src_addr), NULL, 0, NI_NUMERICHOST);
    if (ret != 0)
        i_critical("getnameinfo: %s", gai_strerror(ret));

    sstrncpy(con->r_end_chars, "\n", CON_REMOTE_END_SIZE);

    /* Lookup any blacklists based on this client's src IP address. */
    if ((bl_names = Hash_keys(state->blacklists)) != NULL) {
        LIST_EACH(bl_names, entry)
        {
            bl_name = List_entry_value(entry);
            blacklist = Hash_get(state->blacklists, bl_name);
            IP_sockaddr_to_addr(&con->src, &ipaddr);

            if (Blacklist_match(blacklist, &ipaddr,
                    ((struct sockaddr*)&con->src)->sa_family)) {
                /* Make a local copy for the connection (excluding the entries). */
                con_blacklist = Blacklist_create(bl_name, blacklist->message, BL_STORAGE_TRIE);
                List_insert_after(con->blacklists, con_blacklist);
            }
        }
        List_destroy(&bl_names);
    }

    state->clients++;
    if (List_size(con->blacklists) > 0) {
        state->black_clients++;
        con->lists = Con_summarize_lists(con);

        /* Abandon stuttering if there are to many blacklisted connections. */
        if (greylist && (state->black_clients > state->max_black))
            con->stutter = 0;
    } else {
        con->lists = NULL;
    }

    /* Initialize state machine. */
    con->out_p = con->out_buf;
    con->out_remaining = 0;
    con->in_p = con->in_buf;
    con->in_remaining = 0;
    con->state = state->proxy_protocol_enabled
        ? CON_STATE_PROXY_IN
        : CON_STATE_BANNER_IN;

    time(&now);
    Con_next_state(con, &now, state);
}

extern void
Con_close(struct Con* con, struct Greyd_state* state)
{
    time_t now;

    close(con->fd);
    con->fd = -1;
    state->slow_until = 0;

    time(&now);
    i_info("%s: disconnected after %lld seconds.%s%s",
        con->src_addr, (long long)(now - con->s),
        (List_size(con->blacklists) > 0 ? " lists: " : ""),
        (List_size(con->blacklists) > 0 ? con->lists : ""));

    if (con->lists != NULL) {
        free(con->lists);
        con->lists = NULL;
    }

    if (List_size(con->blacklists) > 0) {
        List_remove_all(con->blacklists);
        state->black_clients--;
    }

    if (con->out_buf != NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_p = NULL;
        con->out_size = 0;
    }

    state->clients--;
}

extern char* Con_summarize_lists(struct Con* con)
{
    char* lists;
    int out_size, first = 1;
    struct List_entry* entry;
    Blacklist_T blacklist;

    if (List_size(con->blacklists) == 0)
        return NULL;

    if ((lists = malloc(CON_BL_SUMMARY_SIZE + 1)) == NULL)
        i_critical("malloc: %s", strerror(errno));
    *lists = '\0';

    out_size = CON_BL_SUMMARY_SIZE - strlen(CON_BL_SUMMARY_ETC);
    LIST_EACH(con->blacklists, entry)
    {
        blacklist = List_entry_value(entry);

        if (strlen(lists) + strlen(blacklist->name) + 1 >= out_size) {
            sstrncat(lists, CON_BL_SUMMARY_ETC, CON_BL_SUMMARY_SIZE + 1);
            break;
        } else {
            if (!first)
                sstrncat(lists, " ", CON_BL_SUMMARY_SIZE + 1);
            sstrncat(lists, blacklist->name, CON_BL_SUMMARY_SIZE + 1);
        }
        first = 0;
    }

    return lists;
}

extern char* Con_grow_out_buf(struct Con* con, int off)
{
    char* out_buf;

    out_buf = realloc(con->out_buf, con->out_size + CON_OUT_BUF_SIZE);
    if (out_buf == NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_size = 0;
        return NULL;
    } else {
        con->out_size += CON_OUT_BUF_SIZE;
        con->out_buf = out_buf;
        return con->out_buf + off;
    }
}

extern void
Con_handle_read(struct Con* con, time_t* now, struct Greyd_state* state)
{
    int end = 0, nread;

    if (con->r) {
        nread = read(con->fd, con->in_p, con->in_remaining);
        switch (nread) {
        case -1:
            i_warning("connection read error");
            /* Fallthrough. */

        case 0:
            Con_close(con, state);
            break;

        default:
            con->in_p[nread] = '\0';
            if (con->r_end_chars[0] && strpbrk(con->in_p, con->r_end_chars)) {
                /* The input now contains a designated end char. */
                end = 1;
            }
            con->in_p += nread;
            con->in_remaining -= nread;
            break;
        }
    }

    if (end || con->in_remaining == 0) {
        /* Replace trailing new lines with a null byte. */
        while (con->in_p > con->in_buf
            && (con->in_p[-1] == '\r' || con->in_p[-1] == '\n')) {
            con->in_p--;
        }
        *(con->in_p) = '\0';
        con->r = 0;
        Con_next_state(con, now, state);
    }
}

extern void
Con_handle_write(struct Con* con, time_t* now, struct Greyd_state* state)
{
    int nwritten, within_max, to_be_written;
    int greylist = Config_get_int(state->config, "enable", "grey",
        GREYLISTING_ENABLED);
    int grey_stutter = Config_get_int(state->config, "stutter", "grey",
        CON_GREY_STUTTER);

    /*
     * Greylisted connections should have their stutter
     * stopped after initial delay.
     */
    if (con->stutter && greylist && List_size(con->blacklists) == 0
        && (*now - con->s) > grey_stutter) {
        con->stutter = 0;
    }

    if (con->w) {
        if (*(con->out_p) == '\n' && !con->seen_cr) {
            /* We must write a \r before a \n. */
            nwritten = write(con->fd, "\r", 1);
            switch (nwritten) {
            case -1:
                i_warning("connection write error");
                /* Fallthrough. */

            case 0:
                Con_close(con, state);
                goto handled;
                break;
            }
        }
        con->seen_cr = (*(con->out_p) == '\r' ? 1 : 0);

        /*
         * Determine whether to write out the remaining output buffer or
         * continue with a byte at a time.
         */
        within_max = (state->clients + CON_CLIENT_TOLERENCE)
            < state->max_cons;
        to_be_written = (within_max && con->stutter ? 1 : con->out_remaining);

        nwritten = write(con->fd, con->out_p, to_be_written);
        switch (nwritten) {
        case -1:
            i_warning("connection write error");
            /* Fallthrough. */

        case 0:
            Con_close(con, state);
            break;

        default:
            con->out_p += nwritten;
            con->out_remaining -= nwritten;
            break;
        }
    }

handled:
    con->w = *now + con->stutter;
    if (con->out_remaining == 0) {
        con->w = 0;
        Con_next_state(con, now, state);
    }
}

extern void
Con_next_state(struct Con* con, time_t* now, struct Greyd_state* state)
{
    char *p, *q, *human_time;
    char email_addr[GREY_MAX_MAIL];
    char* hostname = Config_get_str(state->config, "hostname", NULL, NULL);
    char* error_code = Config_get_str(state->config, "error_code", NULL, CON_ERROR_CODE);
    int greylist = Config_get_int(state->config, "enable", "grey", GREYLISTING_ENABLED);
    int verbose = Config_get_int(state->config, "verbose", NULL, 0);
    int window = Config_get_int(state->config, "window", NULL, 0);
    int next_state;

    if (match(con->in_buf, "QUIT") && con->state < CON_STATE_CLOSE) {
        snprintf(con->out_buf, con->out_size, "221 %s\r\n", hostname);
        con->out_p = con->out_buf;
        con->out_remaining = strlen(con->out_p);
        con->w = *now + con->stutter;
        con->last_state = con->state;
        con->state = CON_STATE_CLOSE;
        return;
    }

    if (match(con->in_buf, "RSET") && con->state > CON_STATE_HELO_OUT
        && con->state < CON_STATE_DATA_IN) {
        snprintf(con->out_buf, con->out_size,
            "250 OK\r\n");
        con->out_p = con->out_buf;
        con->out_remaining = strlen(con->out_p);
        con->w = *now + con->stutter;
        con->last_state = con->state;
        con->state = CON_STATE_HELO_OUT;
        return;
    }

    switch (con->state) {
    case CON_STATE_PROXY_IN:
        con->in_p = con->in_buf;
        con->in_remaining = sizeof(con->in_buf) - 1;
        con->last_state = con->state;
        con->state = CON_STATE_PROXY_OUT;
        con->r = *now;
        break;

    case CON_STATE_PROXY_OUT:
        if (match(con->in_buf, "PROXY")) {
            /*
             * TODO: parse proxy protocol v1 string from:
             *
             *  http://www.haproxy.org/download/1.8/doc/proxy-protocol.txt
             *
             *  PROXY {TCP4|TCP6|UNKNOWN} <client-ip-addr> <dest-ip-addr> <client-port> <dest-port><CR><LF>
             */
            break;
        }
        goto done;

    case CON_STATE_BANNER_IN:
        if ((human_time = strdup(ctime(now))) == NULL)
            i_critical("strdup failed");

        /* Replace the newline with a \0. */
        human_time[strlen(human_time) - 1] = '\0';
        snprintf(con->out_buf, con->out_size, "220 %s ESMTP %s; %s\r\n",
            Config_get_str(state->config, "hostname", NULL, NULL),
            Config_get_str(state->config, "banner", NULL, GREYD_BANNER),
            human_time);
        free(human_time);

        con->out_p = con->out_buf;
        con->out_remaining = strlen(con->out_p);
        con->w = *now + con->stutter;
        con->s = *now;
        con->last_state = con->state;
        con->state = CON_STATE_BANNER_OUT;
        break;

    case CON_STATE_BANNER_OUT:
        con->in_p = con->in_buf;
        con->in_remaining = sizeof(con->in_buf) - 1;
        con->last_state = con->state;
        con->state = CON_STATE_HELO_IN;
        con->r = *now;
        break;

    case CON_STATE_HELO_IN:
        if (match(con->in_buf, "HELO") || match(con->in_buf, "EHLO")) {
            next_state = CON_STATE_HELO_OUT;
            con->helo[0] = '\0';
            get_helo(con->helo, sizeof(con->helo), con->in_buf);
            if (*con->helo == '\0') {
                next_state = CON_STATE_BANNER_OUT;
                snprintf(con->out_buf, con->out_size,
                    "501 Syntax: %s hostname\r\n",
                    match(con->in_buf, "HELO") ? "HELO" : "EHLO");
            } else {
                snprintf(con->out_buf, con->out_size, "250 %s\r\n", hostname);
            }
            con->out_p = con->out_buf;
            con->out_remaining = strlen(con->out_p);
            con->last_state = con->state;
            con->state = next_state;
            con->w = *now + con->stutter;
            break;
        }
        goto mail;

    case CON_STATE_HELO_OUT:
        /* Sent 250 Hello, wait for input. */
        con->in_p = con->in_buf;
        con->in_remaining = sizeof(con->in_buf) - 1;
        con->last_state = con->state;
        con->state = CON_STATE_MAIL_IN;
        con->r = *now;
        break;

    case CON_STATE_MAIL_IN:
    mail:
        if (match(con->in_buf, "MAIL")) {
            set_log(email_addr, sizeof(email_addr), con->in_buf);
            normalize_email_addr(email_addr, con->mail, sizeof(con->mail));
            snprintf(con->out_buf, con->out_size, "250 OK\r\n");
            con->out_p = con->out_buf;
            con->out_remaining = strlen(con->out_p);
            con->last_state = con->state;
            con->state = CON_STATE_MAIL_OUT;
            con->w = *now + con->stutter;
            break;
        }
        goto rcpt;

    case CON_STATE_MAIL_OUT:
        /* Sent 250 Sender ok */
        con->in_p = con->in_buf;
        con->in_remaining = sizeof(con->in_buf) - 1;
        con->last_state = con->state;
        con->state = CON_STATE_RCPT_IN;
        con->r = *now;
        break;

    case CON_STATE_RCPT_IN:
    rcpt:
        if (match(con->in_buf, "RCPT")) {
            set_log(email_addr, sizeof(email_addr), con->in_buf);
            normalize_email_addr(email_addr, con->rcpt, sizeof(con->rcpt));
            snprintf(con->out_buf, con->out_size, "250 OK\r\n");
            con->out_p = con->out_buf;
            con->out_remaining = strlen(con->out_p);
            con->last_state = con->state;
            con->state = CON_STATE_RCPT_OUT;
            con->w = *now + con->stutter;

            if (*con->mail && *con->rcpt) {
                i_debug("(%s) %s: %s -> %s",
                    List_size(con->blacklists) > 0 ? "BLACK" : "GREY",
                    con->src_addr, con->mail, con->rcpt);

                if (greylist && List_size(con->blacklists) == 0) {
                    /*
                     * Send this information to the greylister.
                     */
                    Con_get_orig_dst(con, state);
                    fprintf(state->grey_out,
                        "type = %d\n"
                        "dst_ip = \"%s\"\n"
                        "ip = \"%s\"\n"
                        "helo = \"%s\"\n"
                        "from = \"%s\"\n"
                        "to = \"%s\"\n"
                        "%%\n",
                        GREY_MSG_GREY, con->dst_addr,
                        con->src_addr, con->helo,
                        con->mail, con->rcpt);
                    fflush(state->grey_out);
                }
            } else {
                i_debug("incomplete sender and/or recipient; "
                        "not sending to greylister");
            }
            break;
        }
        goto spam;

    case CON_STATE_RCPT_OUT:
        /* Sent 250. */
        con->in_p = con->in_buf;
        con->in_remaining = sizeof(con->in_buf) - 1;
        con->last_state = con->state;
        con->state = CON_STATE_RCPT_IN;
        con->r = *now;
        break;

    case CON_STATE_DATA_IN:
    spam:
        if (match(con->in_buf, "DATA")) {
            snprintf(con->out_buf, con->out_size,
                "354 End data with <CR><LF>.<CR><LF>\r\n");
            con->state = CON_STATE_DATA_OUT;

            if (window && setsockopt(con->fd, SOL_SOCKET, SO_RCVBUF, &window, sizeof(window)) == -1) {
                /* Don't fail if the above didn't work. */
                i_debug("setsockopt failed, window size of %d", window);
            }

            con->in_p = con->in_buf;
            con->in_remaining = sizeof(con->in_buf) - 1;
            con->out_p = con->out_buf;
            con->out_remaining = strlen(con->out_p);
            con->w = *now + con->stutter;
            if (greylist && List_size(con->blacklists) == 0) {
                con->last_state = con->state;
                con->state = CON_STATE_REPLY;
                goto done;
            }
        } else {
            if (match(con->in_buf, "NOOP")) {
                snprintf(con->out_buf, con->out_size,
                    "250 OK\r\n");
            } else {
                snprintf(con->out_buf, con->out_size,
                    "500 Command unrecognized\r\n");
                con->bad_cmd++;
                if (con->bad_cmd > CON_MAX_BAD_CMD) {
                    con->last_state = con->state;
                    con->state = CON_STATE_REPLY;
                    goto done;
                }
            }

            con->state = con->last_state;
            con->in_p = con->in_buf;
            con->in_remaining = sizeof(con->in_buf) - 1;
            con->out_p = con->out_buf;
            con->out_remaining = strlen(con->out_p);
            con->w = *now + con->stutter;
        }
        break;

    case CON_STATE_DATA_OUT:
        /* Sent 354. */
        con->in_p = con->in_buf;
        con->in_remaining = sizeof(con->in_buf) - 1;
        con->last_state = con->state;
        con->state = CON_STATE_MESSAGE;
        con->r = *now;
        break;

    case CON_STATE_MESSAGE:
        /* Process the message body. */
        for (p = q = con->in_buf; q <= con->in_p; ++q) {
            if (*q == '\n' || q == con->in_p) {
                *q = 0;
                if (q > p && q[-1] == '\r')
                    q[-1] = 0;
                if (!strcmp(p, ".") || (con->data_body && ++con->data_lines >= 10)) {
                    con->last_state = con->state;
                    con->state = CON_STATE_REPLY;
                    goto done;
                }

                if (!con->data_body && !*p)
                    con->data_body = 1;

                if (verbose && con->data_body && *p) {
                    i_info("%s: Body: %s", con->src_addr, p);
                } else if (verbose
                    && (match(p, "FROM:") || match(p, "TO:")
                        || match(p, "SUBJECT:"))) {
                    i_info("%s: %s", con->src_addr, p);
                }

                p = ++q;
            }
        }
        con->in_p = con->in_buf;
        con->in_remaining = sizeof(con->in_buf) - 1;
        con->r = *now;
        break;

    case CON_STATE_REPLY:
    done:
        Con_build_reply(con, error_code);
        con->w = *now + con->stutter;
        con->last_state = con->state;
        con->state = CON_STATE_CLOSE;
        break;

    case CON_STATE_CLOSE:
        Con_close(con, state);
        break;

    default:
        errx(1, "illegal state %d", con->state);
        break;
    }
}

extern int
Con_append_error_string(struct Con* con, size_t off, char* fmt,
    char* error_code, int* last_line_cont)
{
    char* format = fmt;
    char* error_str = con->out_buf + off;
    char saved = '\0';
    size_t error_str_len = con->out_size - off;
    int appended = 0;
    int error_code_len = (strlen(error_code) + 1);

    if (off == 0)
        *last_line_cont = 0;

    if (*last_line_cont != 0)
        con->out_buf[*last_line_cont] = '-';

    if (error_str_len < error_code_len
        && (error_str = Con_grow_out_buf(con, off)) != NULL) {
        error_str_len = con->out_size - off;
    }

    snprintf(error_str, error_str_len, "%s ", error_code);
    appended += strlen(error_str);
    *last_line_cont = off + appended - 1;

    while (*format) {
        /*
         * Each iteration must ensure there is enough space
         * to store a 4 byte response code, a 39 byte IPv6 address,
         * a saved previous byte.
         */
        if (appended >= (error_str_len - (INET6_ADDRSTRLEN + error_code_len + 1))) {
            if ((error_str = Con_grow_out_buf(con, off)) == NULL)
                return -1;
            error_str_len = con->out_size - (off + appended);
        }

        /*
         * If this line is a continuation, we must insert a dash in the previous
         * line, and write the response code again at the beginning of the
         * new line.
         */
        if (error_str[appended - 1] == '\n') {
            if (*last_line_cont)
                con->out_buf[*last_line_cont] = '-';
            snprintf(error_str + appended, error_str_len, "%s ", error_code);
            appended += strlen(error_str + appended);
            *last_line_cont = off + appended - 1;
        }

        switch (*format) {
        case '\\':
        case '%':
            if (!saved) {
                saved = *format;
            } else {
                error_str[appended++] = saved;
                saved = '\0';
                error_str[appended] = '\0';
            }
            break;

        case 'A':
        case 'n':
            if (saved == '\\' && *format == 'n') {
                error_str[appended++] = '\n';
                saved = '\0';
                error_str[appended] = '\0';
                break;
            } else if (saved == '%' && *format == 'A') {
                strncpy(error_str + appended, con->src_addr, sizeof(con->src_addr));
                appended += strlen(error_str + appended);
                saved = '\0';
                error_str[appended] = '\0';
                break;
            }
            /* Fallthrough. */

        default:
            if (saved)
                error_str[appended++] = saved;
            error_str[appended++] = *format;
            saved = '\0';
            error_str[appended] = '\0';
            break;
        }

        format++;
    }

    return appended;
}

extern void
Con_build_reply(struct Con* con, char* error_code)
{
    int off = 0, last_line_cont = 0, remaining, appended;
    char* reply;
    struct List_entry* entry;
    Blacklist_T blacklist;

    if (List_size(con->blacklists) > 0) {
        /*
         * For blacklisted connections, expand and output each
         * blacklist's rejection message, in the SMTP format.
         */
        LIST_EACH(con->blacklists, entry)
        {
            blacklist = List_entry_value(entry);
            appended = 0;
            reply = con->out_buf + off;
            remaining = con->out_size - off;

            appended = Con_append_error_string(con, off, blacklist->message,
                error_code, &last_line_cont);
            if (appended == -1)
                goto error;

            off += appended;
            remaining -= appended;

            if (con->out_buf[off - 1] != '\n') {
                if (remaining < 1 && (reply = Con_grow_out_buf(con, off)) == NULL) {
                    goto error;
                }

                con->out_buf[off++] = '\n';
                con->out_buf[off] = '\0';
                con->out_p = con->out_buf;
                con->out_remaining = strlen(con->out_buf);
            }
        }

        return;
    } else {
        /*
         * This connection is not on any blacklists, so
         * give a generic reply. Note, greylisted connections will
         * always receive a 451.
         */
        snprintf(con->out_buf, con->out_size,
            "451 Temporary failure, please try again later.\r\n");
        con->out_p = con->out_buf;
        con->out_remaining = strlen(con->out_p);
        return;
    }

error:
    if (con->out_buf != NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_size = 0;
    }
}

extern void
Con_get_orig_dst(struct Con* con, struct Greyd_state* state)
{
    struct sockaddr_storage ss_proxy;
    socklen_t proxy_len = sizeof(ss_proxy);
    char proxy[INET6_ADDRSTRLEN], *dst;
    Lexer_source_T source;
    Lexer_T lexer;
    Config_parser_T parser;
    Config_T message;
    struct pollfd fd;
    unsigned short src_port, proxy_port;
    int read_fd;

    if (state->proxy_protocol_enabled) {
        /*
         * When the proxy protocol is enabled we obtain the destination
         * address from the obtained proxy info line, so don't bother poking
         * into the NAT table here.
         */
        return;
    }

    if (getsockname(con->fd, (struct sockaddr*)&ss_proxy, &proxy_len) == -1)
        return;

    if (getnameinfo((struct sockaddr*)&ss_proxy,
            IP_SOCKADDR_LEN(((struct sockaddr*)&ss_proxy)),
            proxy, sizeof(proxy),
            NULL, 0, NI_NUMERICHOST)
        != 0) {
        return;
    }

    if (((struct sockaddr*)&con->src)->sa_family == AF_INET) {
        src_port = ((struct sockaddr_in*)&con->src)->sin_port;
        proxy_port = ((struct sockaddr_in*)&ss_proxy)->sin_port;
    } else {
        src_port = ((struct sockaddr_in6*)&con->src)->sin6_port;
        proxy_port = ((struct sockaddr_in6*)&ss_proxy)->sin6_port;
    }

    fprintf(state->fw_out,
        "type=\"nat\"\n"
        "src=\"%s\"\n"
        "src_port=%u\n"
        "proxy=\"%s\"\n"
        "proxy_port=%u\n%%\n",
        con->src_addr, src_port,
        proxy, proxy_port);
    if (fflush(state->fw_out) == EOF)
        return;

    /* Fetch response from firewall thread. */
    *con->dst_addr = '\0';

    /*
     * Duplicate the descriptor so that the pipe isn't closed
     * upon destroying the lexer source.
     */
    if ((read_fd = dup(fileno(state->fw_in))) == -1)
        i_critical("dup: %s", strerror(errno));

    memset(&fd, 0, sizeof(fd));
    fd.fd = read_fd;
    fd.events = POLLIN;
    if (poll(&fd, 1, DNAT_LOOKUP_TIMEOUT) == -1) {
        i_warning("poll original dst: %s", strerror(errno));
        return;
    }

    if (fd.revents & POLLIN) {
        source = Lexer_source_create_from_fd(fd.fd);
        lexer = Config_lexer_create(source);
        message = Config_create();
        parser = Config_parser_create(lexer);

        if (Config_parser_start(parser, message) == CONFIG_PARSER_OK) {
            dst = Config_get_str(message, "dst", NULL, "");
            sstrncpy(con->dst_addr, dst, sizeof(con->dst_addr));
        }

        Config_destroy(&message);
        Config_parser_destroy(&parser);
    }
}

extern void
Con_accept(int fd, struct sockaddr_storage* addr, struct Greyd_state* state)
{
    int i;
    struct Con* con;

    if (fd == -1) {
        switch (errno) {
        case EINTR:
        case ECONNABORTED:
            break;

        case EMFILE:
        case ENFILE:
            state->slow_until = time(NULL) + 1;
            break;

        default:
            i_critical("accept failure");
        }
    } else {
        /* Ensure we don't hit the configured fd limit. */
        for (i = 0; i < state->max_cons; i++) {
            if (state->cons[i].fd == -1)
                break;
        }

        if (i == state->max_cons) {
            close(fd);
            state->slow_until = 0;
        } else {
            con = &state->cons[i];
            Con_init(con, fd, addr, state);
            i_info("%s: connected (%d/%d)%s%s",
                con->src_addr, state->clients, state->black_clients,
                (con->lists == NULL ? "" : ", lists: "),
                (con->lists == NULL ? "" : con->lists));
        }
    }
}

static int
match(const char* a, const char* b)
{
    return (strncasecmp(a, b, strlen(b)) == 0);
}

static void
get_helo(char* p, size_t len, char* f)
{
    char* s;

    /* Skip HELO/EHLO. */
    f += 4;

    /* Skip whitespace. */
    while (*f == ' ' || *f == '\t')
        f++;
    s = strsep(&f, " \t");
    if (s == NULL)
        return;

    sstrncpy(p, s, len);
    s = strsep(&p, " \t\n\r");
    if (s == NULL)
        return;

    s = strsep(&p, " \t\n\r");
    if (s)
        *s = '\0';
}

static void
set_log(char* p, size_t len, char* f)
{
    char* s;

    s = strsep(&f, ":");
    if (!f) {
        *p = '\0';
        return;
    }

    while (*f == ' ' || *f == '\t')
        f++;
    s = strsep(&f, " \t");
    if (s == NULL)
        return;

    sstrncpy(p, s, len);
    s = strsep(&p, " \t\n\r");
    if (s == NULL)
        return;

    s = strsep(&p, " \t\n\r");
    if (s)
        *s = '\0';
}

static void
destroy_blacklist(void* value)
{
    Blacklist_T bl = (Blacklist_T)value;
    if (bl) {
        Blacklist_destroy(&bl);
    }
}
