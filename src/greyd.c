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

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "blacklist.h"
#include "config_parser.h"
#include "constants.h"
#include "failures.h"
#include "firewall.h"
#include "greyd.h"
#include "greyd_config.h"
#include "hash.h"
#include "ip.h"
#include "list.h"
#include "utils.h"

#define MSG_TYPE_NAT "nat"
#define MSG_TYPE_REPLACE "replace"

#define CMP(a, b) strncmp((a), (b), sizeof((b)))

extern void
Greyd_set_proxy_protocol_permitted_proxies(List_T cidrs, struct Greyd_state* state)
{
    struct List_entry* cur;
    Config_value_T val;
    char* cidr;

    state->proxy_protocol_permitted_proxies = Blacklist_create(
        "permitted-proxies", "permitted upstream proxies", BL_STORAGE_LIST);
    if (cidrs == NULL || List_size(cidrs) == 0) {
        i_warning("no permitted proxies configured, refusing to serve requests");
        return;
    }

    LIST_EACH(cidrs, cur)
    {
        val = List_entry_value(cur);
        if ((cidr = cv_str(val)) == NULL)
            continue;
        i_info("allowing upstream proxy -> %s", cidr);
        Blacklist_add(state->proxy_protocol_permitted_proxies, cidr);
    }
}

extern void
Greyd_process_config(int fd, struct Greyd_state* state)
{
    Config_T message;
    Config_value_T value;
    Lexer_source_T source;
    Lexer_T lexer;
    Config_parser_T parser;
    Blacklist_T blacklist;
    char *bl_name, *bl_msg, *addr;
    List_T ips;
    struct List_entry* entry;
    int read_fd;

    /*
     * Duplicate the descriptor so that the incoming fd isn't closed
     * upon destroying the lexer source.
     */
    if ((read_fd = dup(fd)) == -1)
        i_critical("dup: %s", strerror(errno));
    source = Lexer_source_create_from_fd(read_fd);
    lexer = Config_lexer_create(source);
    parser = Config_parser_create(lexer);
    message = Config_create();

    if (Config_parser_start(parser, message) == CONFIG_PARSER_OK) {
        /*
         * Create a new blacklist and overwrite any existing.
         */
        bl_name = Config_get_str(message, "name", NULL, NULL);
        bl_msg = Config_get_str(message, "message", NULL, NULL);
        ips = Config_get_list(message, "ips", NULL);
        if (bl_name && bl_msg && ips) {
            blacklist = Blacklist_create(bl_name, bl_msg, BL_STORAGE_TRIE);
            LIST_EACH(ips, entry)
            {
                value = List_entry_value(entry);
                if ((addr = cv_str(value)) != NULL)
                    Blacklist_add(blacklist, addr);
            }
            Hash_insert(state->blacklists, bl_name, blacklist);
        }
    }

    Config_destroy(&message);
    Config_parser_destroy(&parser);
}

extern void
Greyd_send_config(FILE* out, char* bl_name, char* bl_msg, List_T ips)
{
    struct List_entry* entry;
    char* ip;
    int first = 1;
    short af;

    if (List_size(ips) > 0) {
        fprintf(out, "name=\"%s\"\nmessage=\"%s\"\nips=[",
            bl_name, bl_msg);

        LIST_EACH(ips, entry)
        {
            ip = List_entry_value(entry);
            if (strrchr(ip, '/') != NULL) {
                /* This contains a CIDR range already. */
                fprintf(out, "%s\"%s\"", (first ? "" : ","), ip);
            } else {
                af = IP_check_addr(ip);
                fprintf(out, "%s\"%s/%d\"", (first ? "" : ","),
                    ip, (af == AF_INET ? 32 : 128));
            }
            first = 0;
        }
        fprintf(out, "]\n%%\n");

        if (fflush(out) == EOF)
            i_debug("configure_greyd: fflush failed");
    }
}

extern void
Greyd_process_fw_message(Config_T message, FW_handle_T fw_handle, FILE* out)
{
    char *type, *src, *proxy, dst[INET6_ADDRSTRLEN];
    struct sockaddr_storage ss_src, ss_proxy, ss_dst;
    struct addrinfo hints, *res;
    unsigned short src_port = 0, proxy_port = 0;
    Config_value_T value;
    struct List_entry* entry;
    char *addr, *name;
    List_T whitelist, ips;
    short af;

    if ((type = Config_get_str(message, "type", NULL, NULL)) == NULL)
        return;

    if (CMP(type, MSG_TYPE_NAT) == 0) {
        /*
         * Perform a DNAT lookup and return the original destination.
         */
        src = Config_get_str(message, "src", NULL, "");
        proxy = Config_get_str(message, "proxy", NULL, "");
        src_port = Config_get_int(message, "src_port", NULL, 0);
        proxy_port = Config_get_int(message, "proxy_port", NULL, 0);
        if (src_port == 0 || proxy_port == 0) {
            i_debug("nat lookup: expecting non-zero src & proxy ports");
            return;
        }

        memset(dst, 0, sizeof(dst));
        memset(&ss_src, 0, sizeof(ss_src));
        memset(&ss_proxy, 0, sizeof(ss_proxy));
        memset(&ss_dst, 0, sizeof(ss_dst));
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM; /* Dummy. */
        hints.ai_protocol = IPPROTO_UDP; /* Dummy. */
        hints.ai_flags = AI_NUMERICHOST;

        if (getaddrinfo(src, NULL, &hints, &res) != 0) {
            i_debug("getaddrinfo: %s", strerror(errno));
            return;
        }
        memcpy(&ss_src, res->ai_addr, res->ai_addrlen);
        free(res);

        if (getaddrinfo(proxy, NULL, &hints, &res) != 0) {
            i_debug("getaddrinfo: %s", strerror(errno));
            return;
        }
        memcpy(&ss_proxy, res->ai_addr, res->ai_addrlen);
        free(res);

        if (((struct sockaddr*)&ss_src)->sa_family == AF_INET) {
            ((struct sockaddr_in*)&ss_src)->sin_port = src_port;
            ((struct sockaddr_in*)&ss_proxy)->sin_port = proxy_port;
        } else {
            ((struct sockaddr_in6*)&ss_src)->sin6_port = src_port;
            ((struct sockaddr_in6*)&ss_proxy)->sin6_port = proxy_port;
        }

        if (FW_lookup_orig_dst(fw_handle,
                (struct sockaddr*)&ss_src,
                (struct sockaddr*)&ss_proxy,
                (struct sockaddr*)&ss_dst)
            == -1) {
            return;
        }

        if (getnameinfo((struct sockaddr*)&ss_dst,
                IP_SOCKADDR_LEN(((struct sockaddr*)&ss_dst)),
                dst, sizeof(dst),
                NULL, 0, NI_NUMERICHOST)
            != 0) {
            dst[0] = '\0';
        }

        fprintf(out, "dst=\"%s\"\n%%\n", dst);
        if (fflush(out) == EOF)
            i_debug("dnat lookup: fflush failed");
    } else if (CMP(type, MSG_TYPE_REPLACE) == 0) {
        /*
         * Retrieve list of ip addresses to send to the
         * firewall.
         */
        name = Config_get_str(message, "name", NULL, "");
        af = Config_get_int(message, "af", NULL, AF_INET);
        ips = Config_get_list(message, "ips", NULL);

        if (ips && List_size(ips) > 0) {
            whitelist = List_create(NULL);
            LIST_EACH(ips, entry)
            {
                value = List_entry_value(entry);
                if ((addr = cv_str(value)) != NULL
                    && IP_check_addr(addr) != -1) {
                    List_insert_after(whitelist, addr);
                }
            }

            if (List_size(whitelist) > 0)
                FW_replace(fw_handle, name, whitelist, af);

            List_destroy(&whitelist);
        }
    }
}

extern int
Greyd_start_fw_child(struct Greyd_state* state, int in_fd, int nat_in_fd, int out_fd)
{
    Config_T config = state->config;
    FW_handle_T fw_handle;
    FILE* out;
    Lexer_source_T source, nat_source;
    Lexer_T lexer, nat_lexer;
    Config_parser_T parser;
    Config_T message;
    struct passwd* main_pw;
    char *main_user, *chroot_dir = NULL;
    int ret, i;

    /* Setup the firewall handle before dropping privileges. */
    if ((fw_handle = FW_open(config)) == NULL)
        i_critical("could not obtain firewall handle");

    main_user = Config_get_str(config, "user", NULL, GREYD_MAIN_USER);
    if ((main_pw = getpwnam(main_user)) == NULL)
        errx(1, "no such user %s", main_user);

#ifndef WITH_PF
    if (Config_get_int(config, "chroot", NULL, GREYD_CHROOT)) {
        tzset();
        chroot_dir = Config_get_str(config, "chroot_dir", NULL,
            GREYD_CHROOT_DIR);
        if (chroot(chroot_dir) == -1)
            i_critical("cannot chroot to %s", chroot_dir);
    }
#endif

    if (main_pw && Config_get_int(config, "drop_privs", NULL, 1)
        && drop_privs(main_pw) == -1) {
        i_critical("failed to drop privileges: %s", strerror(errno));
    }

    if ((out = fdopen(out_fd, "w")) == NULL)
        i_critical("fdopen: %s", strerror(errno));

    struct pollfd fds[2];
    memset(&fds, 0, 2 * sizeof(*fds));
    fds[0].fd = in_fd;
    fds[0].events = POLLIN;
    fds[1].fd = nat_in_fd;
    fds[1].events = POLLIN;

    nat_source = Lexer_source_create_from_fd(nat_in_fd);
    nat_lexer = Config_lexer_create(nat_source);

    source = Lexer_source_create_from_fd(in_fd);
    lexer = Config_lexer_create(source);
    parser = Config_parser_create(lexer);

    for (;;) {
        if (state->shutdown) {
            i_info("stopping firewall process");
            goto cleanup;
        }

        if ((poll(fds, 2, POLL_TIMEOUT) == -1) && errno != EINTR) {
            i_warning("firewall process, poll error: %s", strerror(errno));
        } else {
            for (i = 0; i < 2; i++) {
                if (fds[i].revents & POLLIN) {
                    Config_parser_set_lexer(
                        parser, (i == 0 ? lexer : nat_lexer));
                    message = Config_create();
                    ret = Config_parser_start(parser, message);
                    switch (ret) {
                    case CONFIG_PARSER_OK:
                        Greyd_process_fw_message(message, fw_handle, out);
                        break;

                    case CONFIG_PARSER_ERR:
                        i_warning("firewall process: parse error");
                        break;
                    }

                    Config_destroy(&message);
                }
            }
        }
    }

cleanup:
    Lexer_destroy(&lexer);
    Lexer_destroy(&nat_lexer);
    Config_parser_set_lexer(parser, NULL);
    Config_parser_destroy(&parser);
    FW_close(&fw_handle);
    Config_destroy(&config);

    return 0;
}
