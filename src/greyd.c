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
 * @brief  Implements greyd support constants and definitions.
 * @author Mikey Austin
 * @date   2014
 */

#include <sys/socket.h>

#include <errno.h>
#include <string.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include "failures.h"
#include "greyd_config.h"
#include "greyd.h"
#include "hash.h"
#include "ip.h"
#include "list.h"
#include "config_parser.h"
#include "blacklist.h"
#include "firewall.h"

#define MSG_TYPE_NAT     "nat"
#define MSG_TYPE_REPLACE "replace"

#define CMP(a, b) strncmp((a), (b), sizeof((b)))

extern void
Greyd_process_config(int fd, struct Greyd_state *state)
{
    Config_T message;
    Config_value_T value;
    Lexer_source_T source;
    Lexer_T lexer;
    Config_parser_T parser;
    Blacklist_T blacklist;
    char *bl_name, *bl_msg, *addr;
    List_T ips;
    struct List_entry *entry;
    int read_fd;

    /*
     * Duplicate the descriptor so that the incoming fd isn't closed
     * upon destroying the lexer source.
     */
    if((read_fd = dup(fd)) == -1)
        i_critical("dup: %s", strerror(errno));
    source = Lexer_source_create_from_fd(read_fd);
    lexer = Config_lexer_create(source);
    parser = Config_parser_create(lexer);
    message = Config_create();

    if(Config_parser_start(parser, message) == CONFIG_PARSER_OK) {
        /*
         * Create a new blacklist and overwrite any existing.
         */
        bl_name = Config_get_str(message, "name", NULL, NULL);
        bl_msg = Config_get_str(message, "message", NULL, NULL);
        ips = Config_get_list(message, "ips", NULL);
        if(bl_name && bl_msg && ips) {
            blacklist = Blacklist_create(bl_name, bl_msg);
            LIST_EACH(ips, entry) {
                value = List_entry_value(entry);
                if((addr = cv_str(value)) != NULL)
                    Blacklist_add(blacklist, addr);
            }
            Hash_insert(state->blacklists, bl_name, blacklist);
        }
    }

    Config_destroy(&message);
    Config_parser_destroy(&parser);
}

extern void
Greyd_send_config(FILE *out, char *bl_name, char *bl_msg, List_T ips)
{
    struct List_entry *entry;
    char *ip;
    int first = 1;
    short af;

    if(List_size(ips) > 0) {
        fprintf(out, "name=\"%s\"\nmessage=\"%s\"\nips=[",
                bl_name, bl_msg);

        LIST_EACH(ips, entry) {
            ip = List_entry_value(entry);
            af = IP_check_addr(ip);
            fprintf(out, "%s\"%s/%d\"", (first ? "" : ","),
                    ip, (af == AF_INET ? 32 : 128));
            first = 0;
        }
        fprintf(out, "]\n%%\n");

        if(fflush(out) == EOF)
            i_debug("configure_greyd: fflush failed");
    }
}

extern void
Greyd_process_fw_message(Config_T message, FW_handle_T fw_handle, FILE *out)
{
    char *type, *src, *proxy, dst[INET6_ADDRSTRLEN];
    struct sockaddr_storage ss_src, ss_proxy, ss_dst;
    struct addrinfo hints, *res;

    if((type = Config_get_str(message, "type", NULL, NULL)) == NULL)
        return;

    if(CMP(type, MSG_TYPE_NAT) == 0) {
        /*
         * Perform a DNAT lookup and return the original destination.
         */
        src = Config_get_str(message, "src", NULL, "");
        proxy = Config_get_str(message, "proxy", NULL, "");

        memset(dst, 0, sizeof(dst));
        memset(&ss_src, 0, sizeof(ss_src));
        memset(&ss_proxy, 0, sizeof(ss_proxy));
        memset(&ss_dst, 0, sizeof(ss_dst));
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;  /* Dummy. */
        hints.ai_protocol = IPPROTO_UDP; /* Dummy. */
        hints.ai_flags = AI_NUMERICHOST;

        if(getaddrinfo(src, NULL, &hints, &res) != 0)
            return;
        memcpy(&ss_src, res->ai_addr, res->ai_addrlen);
        free(res);

        if(getaddrinfo(proxy, NULL, &hints, &res) != 0)
            return;
        memcpy(&ss_proxy, res->ai_addr, res->ai_addrlen);
        free(res);

        if(FW_lookup_orig_dst(fw_handle,
                              (struct sockaddr *) &ss_src,
                              (struct sockaddr *) &ss_proxy,
                              (struct sockaddr *) &ss_dst) == -1)
        {
            return;
        }

        if(getnameinfo((struct sockaddr *) &ss_dst,
                       IP_SOCKADDR_LEN(((struct sockaddr *) &ss_dst)),
                       dst, sizeof(dst),
                       NULL, 0, NI_NUMERICHOST) != 0)
        {
            dst[0] = '\0';
        }

        fprintf(out, "dst=\"%s\"\n%%\n", dst);
        if(fflush(out) == EOF)
            i_debug("dnat lookup: fflush failed");
    }
    else if(CMP(type, MSG_TYPE_REPLACE) == 0) {
        // TODO: send list of addresses to firewall.
    }
}
