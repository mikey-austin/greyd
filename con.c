/**
 * @file   con.c
 * @brief  Implements the connection management interface.
 * @author Mikey Austin
 * @date   2014
 */

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

#define _GNU_SOURCE

#include "failures.h"
#include "con.h"
#include "blacklist.h"
#include "list.h"
#include "utils.h"
#include "grey.h"

#include <err.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

static int match(const char *, const char *);
static void get_helo(char *, size_t, char *);
static void set_log(char *, size_t, char *);

extern void
Con_init(struct Con *con, int fd, struct sockaddr_storage *src,
         struct Con_list *cons, List_T blacklists, Config_T config)
{
    time_t now;
    short greylist, grey_stutter;
    int ret, max_black;
    char *human_time;
    struct List_entry *entry;
    Blacklist_T blacklist = NULL;
    struct IP_addr ipaddr;

    time(&now);

    /* Free resources before zeroing out this entry. */
    free(con->out_buf);
    con->out_buf = NULL;
    con->out_size = 0;
    List_destroy(&(con->blacklists));
    free(con->lists);
    con->lists = NULL;
    memset(con, 0, sizeof *con);

    /* Start initializing the connection. */
    if(Con_grow_out_buf(con, 0) == NULL)
        I_CRIT("could not grow connection out buf");

    con->blacklists = List_create(NULL);
    con->fd = fd;

    greylist = Config_get_int(config, "enable", "grey", 1);
    grey_stutter = Config_get_int(config, "stutter", "grey", CON_GREY_STUTTER);
    con->stutter = (greylist && !grey_stutter && List_size(con->blacklists) == 0)
        ? 0 : Config_get_int(config, "stutter", NULL, CON_STUTTER);

    memcpy(&(con->src), src, sizeof(*src));
    ret = getnameinfo((struct sockaddr *) src, sizeof(*src), con->src_addr,
                      sizeof(con->src_addr), NULL, 0, NI_NUMERICHOST);
    if(ret != 0)
        I_CRIT("getnameinfo: %s", gai_strerror(ret));

    if((human_time = strdup(ctime(&now))) == NULL)
        I_CRIT("strdup failed");

    /* Replace the newline with a \0. */
    human_time[strlen(human_time) - 1] = '\0';
    snprintf(con->out_buf, con->out_size, "220 %s ESMTP %s; %s\r\n",
             Config_get_str(config, "hostname", NULL, NULL),
             Config_get_str(config, "banner", NULL, NULL),
             human_time);
    free(human_time);

    con->out_p = con->out_buf;
    con->out_remaining = strlen(con->out_p);

    con->in_p = con->in_buf;
    con->in_remaining = 0;

    /* Set the stuttering time values. */
    con->w = now + con->stutter;
    con->s = now;
    sstrncpy(con->r_end_chars, "\n", CON_REMOTE_END_SIZE);

    /* Lookup any blacklists based on this client's src IP address. */
    LIST_FOREACH(blacklists, entry) {
        blacklist = List_entry_value(entry);
        IP_sockaddr_to_addr(&con->src, &ipaddr);

        if(Blacklist_match(blacklist, &ipaddr, ((struct sockaddr *) &con->src)->sa_family))
            List_insert_after(con->blacklists, blacklist);
    }

    cons->clients++;
    if(List_size(con->blacklists) > 0) {
        cons->black_clients++;
        con->lists = Con_summarize_lists(con);

        max_black = Config_get_int(config, "max_black", NULL, CON_DEFAULT_MAX);
        if(greylist && (cons->black_clients > max_black))
            con->stutter = 0;
    }
    else {
        con->lists = NULL;
    }
}

extern void
Con_close(struct Con *con, struct Con_list *cons, int *slow_until)
{
    time_t now;

    close(con->fd);
    con->fd = -1;
    *slow_until = 0;

    time(&now);
    I_INFO("%s: disconnected after %lld seconds.%s%s",
           con->src_addr, (long long) (now - con->s),
           (List_size(con->blacklists) > 0 ? " lists:" : ""),
           (List_size(con->blacklists) > 0 ? con->lists : ""));

    if(con->lists != NULL) {
        free(con->lists);
        con->lists = NULL;
    }

    if(List_size(con->blacklists) > 0) {
        List_remove_all(con->blacklists);
        cons->black_clients--;
    }

    if(con->out_buf != NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_size = 0;
    }

    cons->clients--;
}

extern char
*Con_summarize_lists(struct Con *con)
{
    char *lists;
    int out_size;
    struct List_entry *entry;
    Blacklist_T blacklist;

    if(List_size(con->blacklists) == 0)
        return NULL;

    if((lists = malloc(CON_BL_SUMMARY_SIZE + 1)) == NULL)
        I_CRIT("could not malloc list summary");
    *lists = '\0';

    out_size = CON_BL_SUMMARY_SIZE - strlen(CON_BL_SUMMARY_ETC);
    LIST_FOREACH(con->blacklists, entry) {
        blacklist = List_entry_value(entry);

        if(strlen(lists) + strlen(blacklist->name) + 1 >= out_size) {
            strcat(lists, CON_BL_SUMMARY_ETC);
            break;
        }
        else {
            strcat(lists, " ");
            strcat(lists, blacklist->name);
        }
    }

    return lists;
}

extern char
*Con_grow_out_buf(struct Con *con, int off)
{
    char *out_buf;

    out_buf = realloc(con->out_buf, con->out_size + CON_OUT_BUF_SIZE);
    if(out_buf == NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_size = 0;
        return NULL;
    }
    else {
        con->out_size += CON_OUT_BUF_SIZE;
        con->out_buf = out_buf;
        return con->out_buf + off;
    }
}

extern void
Con_handle_read(struct Con *con, time_t *now, struct Con_list *cons,
                int *slow_until, Config_T config, FILE *grey_out)
{
    int end = 0, nread;

    if(con->r) {
        nread = read(con->fd, con->in_p, con->in_remaining);
        switch(nread) {
        case -1:
            I_WARN("connection read error");
            /* Fallthrough. */

        case 0:
            Con_close(con, cons, slow_until);
            break;

        default:
            con->in_p[nread] = '\0';
            if(con->r_end_chars[0] && strpbrk(con->in_p, con->r_end_chars)) {
                /* The input now contains a designated end char. */
                end = 1;
            }
            con->in_p += nread;
            con->in_remaining -= nread;
            break;
        }
    }

    if(end || con->in_remaining == 0) {
        /* Replace trailing new lines with a null byte. */
        while(con->in_p > con->in_buf
              && (con->in_p[-1] == '\r' || con->in_p[-1] == '\n'))
        {
            con->in_p--;
        }
        *(con->in_p) = '\0';
        con->r = 0;
        Con_next_state(con, now, cons, slow_until, config, grey_out);
    }
}

extern void
Con_handle_write(struct Con *con, time_t *now, struct Con_list *cons,
                 int *slow_until, Config_T config, FILE *grey_out)
{
    int nwritten, within_max, to_be_written;
    int greylist = Config_get_int(config, "enable", "grey", 1);
    int grey_stutter = Config_get_int(config, "stutter", "grey", CON_GREY_STUTTER);

    /*
     * Greylisted connections should have their stutter
     * stopped after initial delay.
     */
    if(con->stutter && greylist && List_size(con->blacklists) == 0
       && (*now - con->s) > grey_stutter)
    {
        con->stutter = 0;
    }

    if(con->w) {
        if(*(con->out_p) == '\n' && !con->seen_cr) {
            /* We must write a \r before a \n. */
            nwritten = write(con->fd, "\r", 1);
            switch(nwritten) {
            case -1:
                I_WARN("connection write error");
                /* Fallthrough. */

            case 0:
                Con_close(con, cons, slow_until);
                goto handled;
                break;
            }
        }
        con->seen_cr = (*(con->out_p) == '\r' ? 1 : 0);

        /*
         * Determine whether to write out the remaining output buffer or
         * continue with a byte at a time.
         */
        within_max = (cons->clients + CON_CLIENT_TOLERENCE) < cons->size;
        to_be_written = (within_max && con->stutter ? 1 : con->out_remaining);

        nwritten = write(con->fd, con->out_p, to_be_written);
        switch(nwritten) {
        case -1:
            I_WARN("connection write error");
            /* Fallthrough. */

        case 0:
            Con_close(con, cons, slow_until);
            break;

        default:
            con->out_p += nwritten;
            con->out_remaining -= nwritten;
            break;
        }
    }

handled:
    con->w = *now + con->stutter;
    if(con->out_remaining == 0) {
        con->w = 0;
        Con_next_state(con, now, cons, slow_until, config, grey_out);
    }
}

extern void
Con_next_state(struct Con *con, time_t *now, struct Con_list *cons,
               int *slow_until, Config_T config, FILE *grey_out)
{
    char *p, *q;
    char *hostname = Config_get_str(config, "hostname", NULL, NULL);
    char *error_code = Config_get_str(config, "error_code", NULL, NULL);
    int greylist = Config_get_int(config, "enable", "grey", 1);
    int verbose = Config_get_int(config, "verbose", NULL, 0);
    int window = Config_get_int(config, "window", NULL, 0);
    int next_state;

    if(match(con->in_buf, "QUIT") && con->state < CON_STATE_CLOSE) {
        snprintf(con->out_buf, con->out_size, "221 %s\r\n", hostname);
        con->out_p = con->out_buf;
        con->out_remaining = strlen(con->out_p);
        con->w = *now + con->stutter;
        con->last_state = con->state;
        con->state = CON_STATE_CLOSE;
        return;
    }

    if(match(con->in_buf, "RSET") && con->state > CON_STATE_HELO_OUT
       && con->state < CON_STATE_DATA_IN)
    {
        snprintf(con->out_buf, con->out_size,
                 "250 OK\r\n");
        con->out_p = con->out_buf;
        con->out_remaining = strlen(con->out_p);
        con->w = *now + con->stutter;
        con->last_state = con->state;
        con->state = CON_STATE_HELO_OUT;
        return;
    }

    switch(con->state) {
    case CON_STATE_BANNER_SENT:
        con->in_p = con->in_buf;
        con->in_remaining = sizeof(con->in_buf) - 1;
        con->last_state = con->state;
        con->state = CON_STATE_HELO_IN;
        con->r = *now;
        break;

    case CON_STATE_HELO_IN:
        if(match(con->in_buf, "HELO") || match(con->in_buf, "EHLO")) {
            next_state = CON_STATE_HELO_OUT;
            con->helo[0] = '\0';
            get_helo(con->helo, sizeof(con->helo), con->in_buf);
            if(*con->helo == '\0') {
                next_state = CON_STATE_BANNER_SENT;
                snprintf(con->out_buf, con->out_size,
                         "501 Syntax: %s hostname\r\n",
                         match(con->in_buf, "HELO") ? "HELO" : "EHLO");
            }
            else {
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
        if(match(con->in_buf, "MAIL")) {
            set_log(con->mail, sizeof(con->mail), con->in_buf);
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
        if(match(con->in_buf, "RCPT")) {
            set_log(con->rcpt, sizeof(con->rcpt), con->in_buf);
            snprintf(con->out_buf, con->out_size, "250 OK\r\n");
            con->out_p = con->out_buf;
            con->out_remaining = strlen(con->out_p);
            con->last_state = con->state;
            con->state = CON_STATE_RCPT_OUT;
            con->w = *now + con->stutter;

            if(*con->mail && *con->rcpt) {
                I_DEBUG("(%s) %s: %s -> %s\n",
                        List_size(con->blacklists) > 0 ? "BLACK" : "GREY",
                        con->src_addr, con->mail, con->rcpt);

                if(verbose)
                    I_INFO("(%s) %s: %s -> %s\n",
                            List_size(con->blacklists) > 0 ? "BLACK" : "GREY",
                            con->src_addr, con->mail, con->rcpt);

                if(greylist && List_size(con->blacklists) == 0) {
                    /*
                     * Send this information to the greylister.
                     */
                    Con_get_orig_dst_addr(con);
                    fprintf(grey_out,
                            "type = %d\n"
                            "dst_ip = \"%s\"\n"
                            "ip = \"%s\"\n"
                            "helo = \"%s\"\n"
                            "from = \"%s\"\n"
                            "to = \"%s\"\n"
                            "%%\n", GREY_MSG_GREY, con->dst_addr,
                            con->src_addr, con->helo,
                            con->mail, con->rcpt);
                    fflush(grey_out);
                }
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
        if(match(con->in_buf, "DATA")) {
            snprintf(con->out_buf, con->out_size,
                     "354 End data with <CR><LF>.<CR><LF>\r\n");
            con->state = CON_STATE_DATA_OUT;

            if(window && setsockopt(con->fd, SOL_SOCKET, SO_RCVBUF,
                                    &window, sizeof(window)) == -1)
            {
                /* Don't fail if the above didn't work. */
                I_DEBUG("setsockopt failed, window size of %d", window);
            }

            con->in_p = con->in_buf;
            con->in_remaining = sizeof(con->in_buf) - 1;
            con->out_p = con->out_buf;
            con->out_remaining = strlen(con->out_p);
            con->w = *now + con->stutter;
            if(greylist && List_size(con->blacklists) == 0) {
                con->last_state = con->state;
                con->state = CON_STATE_REPLY;
                goto done;
            }
        }
        else {
            if(match(con->in_buf, "NOOP")) {
                snprintf(con->out_buf, con->out_size,
                         "250 OK\r\n");
            }
            else {
                snprintf(con->out_buf, con->out_size,
                         "500 Command unrecognized\r\n");
                con->bad_cmd++;
                if(con->bad_cmd > CON_MAX_BAD_CMD) {
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
        for(p = q = con->in_buf; q <= con->in_p; ++q) {
            if(*q == '\n' || q == con->in_p) {
                *q = 0;
                if(q > p && q[-1] == '\r')
                    q[-1] = 0;
                if(!strcmp(p, ".") ||
                   (con->data_body && ++con->data_lines >= 10)) {
                    con->last_state = con->state;
                    con->state = CON_STATE_REPLY;
                    goto done;
                }

                if(!con->data_body && !*p)
                    con->data_body = 1;

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
        con->out_p = con->out_buf;
        con->out_remaining = strlen(con->out_p);
        con->w = *now + con->stutter;
        con->last_state = con->state;
        con->state = CON_STATE_CLOSE;
        break;

    case CON_STATE_CLOSE:
        Con_close(con, cons, slow_until);
        break;

    default:
        errx(1, "illegal state %d", con->state);
        break;
    }
}

extern int
Con_append_error_string(struct Con *con, size_t off, char *fmt,
                        char *response_code, int *last_line_cont)
{
    char *format = fmt;
    char *error_str = con->out_buf + off;
    char saved = '\0';
    size_t error_str_len = con->out_size - off;
    int appended = 0;
    int response_code_len = (strlen(response_code) + 1);

    if(off == 0)
        *last_line_cont = 0;

    if(*last_line_cont != 0)
        con->out_buf[*last_line_cont] = '-';

    if(error_str_len < response_code_len
       && (error_str = Con_grow_out_buf(con, off)) != NULL)
    {
        error_str_len = con->out_size - off;
    }

    snprintf(error_str, error_str_len, "%s ", response_code);
    appended += strlen(error_str);
    *last_line_cont = off + appended - 1;

    while(*error_str) {
        /*
         * Each iteration must ensure there is enough space
         * to store a 4 byte response code, a 39 byte IPv6 address,
         * a saved previous byte.
         */
        if(appended >= (error_str_len - (INET6_ADDRSTRLEN + response_code_len + 1))) {
            if((error_str = Con_grow_out_buf(con, off)) == NULL)
                return -1;
            error_str_len = con->out_size - (off + appended);
        }

        /*
         * If this line is a continuation, we must insert a dash in the previous
         * line, and write the response code again at the beginning of the
         * new line.
         */
        if(error_str[appended - 1] == '\n') {
            if(*last_line_cont)
                con->out_buf[*last_line_cont] = '-';
            snprintf(error_str + appended, error_str_len, "%s ", response_code);
            appended += strlen(error_str);
            *last_line_cont = off + appended - 1;
        }

        switch(*format) {
        case '\\':
        case '%':
            if(!saved) {
                saved = *format;
            }
            else {
                error_str[appended++] = saved;
                saved = '\0';
                error_str[appended] = '\0';
            }
            break;

        case 'A':
        case 'n':
            if(saved == '\\' && *format == 'n') {
                error_str[appended++] = '\n';
                saved = '\0';
                error_str[appended] = '\0';
                break;
            }
            else if(saved == '%' && *format == 'A') {
                strncpy(error_str + appended, con->src_addr, sizeof(con->src_addr));
                appended += strlen(error_str + appended);
                saved = '\0';
                error_str[appended] = '\0';
                break;
            }
            /* Fallthrough. */

        default:
            if(saved)
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
Con_build_reply(struct Con *con, char *response_code)
{
    int off = 0, last_line_cont = 0, remaining, appended;
    char *reply;
    struct List_entry *entry;
    Blacklist_T blacklist;

    if(List_size(con->blacklists) > 0) {
        LIST_FOREACH(con->blacklists, entry) {
            blacklist = List_entry_value(entry);
            appended = 0;
            reply = con->out_buf + off;
            remaining = con->out_size - off;

            appended = Con_append_error_string(con, off, blacklist->message,
                                               response_code, &last_line_cont);
            if(appended == -1)
                goto error;

            off += appended;
            remaining -= appended;

            if(con->out_buf[off - 1] != '\n') {
                if(remaining < 1 &&
                   (reply = Con_grow_out_buf(con, off)) == NULL)
                {
                    goto error;
                }

                con->out_buf[off++] = '\n';
                con->out_buf[off] = '\0';
            }
        }

        return;
    }
    else {
        /*
         * This connection is not on any blacklists, so
         * give a generic reply.
         */
        asprintf(&con->out_buf,
                 "451 Temporary failure, please try again later.\r\n");
        con->out_size = (con->out_buf == NULL
                         ? 0 : strlen(con->out_buf) + 1);
        return;
    }

error:
    if(con->out_buf != NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_size = 0;
    }
}

extern void
Con_get_orig_dst_addr(struct Con *con)
{
    struct sockaddr_storage dst;
    socklen_t dst_len = sizeof(dst);

    *con->dst_addr = '\0';
    if(getsockname(con->fd, (struct sockaddr *) &dst, &dst_len) == -1)
        return;

    // TODO: as connections are DNAT'ed to greyd, we must perform a
    // lookup in the NAT table for this connection's source address,
    // in order to find the original destination address. At the moment,
    // the below will just return localhost (as it is what greyd is
    // listening on).

    if(getnameinfo((struct sockaddr *) &dst, dst_len, con->dst_addr,
                   sizeof(con->dst_addr), NULL, 0, NI_NUMERICHOST) != 0)
    {
        con->dst_addr[0] = '\0';
    }
}

static int
match(const char *a, const char *b)
{
    return (strncasecmp(a, b, strlen(b)) == 0);
}

static void
get_helo(char *p, size_t len, char *f)
{
    char *s;

    /* Skip HELO/EHLO. */
    f += 4;

    /* Skip whitespace. */
    while(*f == ' ' || *f == '\t')
        f++;
    s = strsep(&f, " \t");
    if(s == NULL)
        return;

    sstrncpy(p, s, len);
    s = strsep(&p, " \t\n\r");
    if(s == NULL)
        return;

    s = strsep(&p, " \t\n\r");
    if(s)
        *s = '\0';
}

static void
set_log(char *p, size_t len, char *f)
{
    char *s;

    s = strsep(&f, ":");
    if(!f)
        return;

    while(*f == ' ' || *f == '\t')
        f++;
    s = strsep(&f, " \t");
    if(s == NULL)
        return;

    sstrncpy(p, s, len);
    s = strsep(&p, " \t\n\r");
    if(s == NULL)
        return;

    s = strsep(&p, " \t\n\r");
    if(s)
        *s = '\0';
}