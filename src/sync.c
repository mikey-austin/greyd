/**
 * @file   sync.c
 * @brief  Implements the multicast/UDP sync engine interface.
 * @author Mikey Austin
 * @date   2014
 */

/*
 * Copyright (c) 2014 Mikey Austin <mikey@jackiemclean.net>
 * Copyright (c) 2006, 2007 Reyk Floeter <reyk@openbsd.org>
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

#include <sys/file.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sha1.h>
#include <syslog.h>
#include <stdint.h>

#include <netdb.h>

#include <openssl/hmac.h>
#include <openssl/sha1.h>

#include "failures.h"
#include "constants.h"
#include "grey.h"
#include "sync.h"
#include "list.h"

struct Sync_host {
    char *name;
    struct sockaddr_in addr;
};

static void destroy_sync_host(void *);

extern Sync_engine_T
Sync_init(Config_T config, FILE *grey_out)
{
    Sync_engine_T engine;
    char *key_path;

    if((engine = calloc(1, sizeof(*engine))) == NULL)
        err(1, "calloc");

    engine->sync_fd = -1;
    engine->config = config;
    engine->grey_out = grey_out;
    engine->sync_hosts = List_create(destroy_sync_host);
    engine->port = Config_get_int(config, "port", "sync", GREYD_SYNC_PORT);

    /* Use empty key by default. */
    engine->sync_key = "";
    if(Config_get_int(config, "verify_messages", "sync", SYNC_VERIFY_MSG)) {
        key_path = Config_get_str(config, "key", "sync", SYNC_KEY);
        engine->sync_key = SHA1File(key_path, NULL);
        if(engine->sync_key == NULL) {
            if(errno != ENOENT) {
                i_warning("failed to open sync key: %s",
                          strerror(errno));
                return NULL;
            }
        }
    }

    return engine;
}

extern int
Sync_start(Sync_engine_T engine)
{
    char *bind_addr, *iface;
    int one = 1;
    u_int8_t ttl;
    struct ifreq ifr;
    struct ip_mreq mreq;
    struct sockaddr_in *addr;
    char if_name[IFNAMSIZ], *ttl_str;
    char *end;
    struct in_addr bind_in_addr;

    bind_addr = Config_get_str(engine->config, "bind_address", "sync", NULL);
    iface = Config_get_str(engine->config, "interface", "sync", NULL);

    if(iface != NULL)
        engine->send_mcast++;

    memset(&bind_in_addr, 0, sizeof(bind_in_addr));
    if(bind_addr != NULL) {
        if(inet_pton(AF_INET, bind_addr, &bind_in_addr) != 1) {
            bind_in_addr.s_addr = htonl(INADDR_ANY);
            if(iface == NULL) {
                iface = bind_addr;
            }
            else if(strcmp(bind_addr, iface) != 0) {
                fprintf(stderr, "multicast interface does not match");
                return -1;
            }
        }
    }

    engine->sync_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(engine->sync_fd == -1)
        return -1;

    if(setsockopt(engine->sync_fd, SOL_SOCKET, SO_REUSEADDR, &one,
                  sizeof(one)) == -1)
    {
        goto fail;
    }

    bzero(&sync_out, sizeof(sync_out));
    memset(&engine->sync_out, 0, sizeof(engine->sync_out));
    engine->sync_out.sin_family = AF_INET;
    engine->sync_out.sin_len = sizeof(engine->sync_out);
    engine->sync_out.sin_addr.s_addr = bind_in_addr.s_addr;
    if(bind_addr == NULL && iface == NULL)
        engine->sync_out.sin_port = 0;
    else
        engine->sync_out.sin_port = htons(port);

    if(bind(engine->sync_fd, (struct sockaddr *) &engine->sync_out,
            sizeof(engine->sync_out)) == -1)
    {
        goto fail;
    }

    if(iface == NULL) {
        /* Don't use multicast messages. */
        return engine->sync_fd;
    }

    /* Extract any TTL value from the interface name. */
    sstrncpy(if_name, iface, sizeof(if_name));
    ttl = SYNC_MCASTTTL;
    if((ttl_str = strchr(if_name, ':')) != NULL) {
        *ttl_str++ = '\0';
        errno = 0;
        ttl = strtol(expires, &end, 10);
        if(expire == 0 || ttl_str[0] == '\0' || *end != '\0'
           || (errno == ERANGE && (ttl == LONG_MAX || ttl == LONG_MIN)))
        {
            i_warning("invalid multicast ttl %s", ttl_str);
            goto fail;
        }
    }

    memset(&ifr, 0, sizeof(ifr));
    sstrncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
    if(ioctl(engine->sync_fd, SIOCGIFADDR, &ifr) == -1)
        goto fail;

    memset(&engine->sync_in, 0, sizeof(engine->sync_in));
    addr = (struct sockaddr_in *) &ifr.ifr_addr;
    engine->sync_in.sin_family = AF_INET;
    engine->sync_in.sin_len = sizeof(engine->sync_in);
    engine->sync_in.sin_addr.s_addr = addr->sin_addr.s_addr;
    engine->sync_in.sin_port = htons(engine->port);

    memset(&mreq, 0, sizeof(mreq));
    engine->sync_out.sin_addr.s_addr = inet_addr(SYNC_MCASTADDR);
    mreq.imr_multiaddr.s_addr = inet_addr(SYNC_MCASTADDR);
    mreq.imr_interface.s_addr = engine->sync_in.sin_addr.s_addr;

    if(setsockopt(engine->sync_fd, IPPROTO_IP,
        IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
    {
        i_warning("failed to add multicast membership to %s: %s",
                  SYNC_MCASTADDR, strerror(errno));
        goto fail;
    }

    if(setsockopt(engine->sync_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
        sizeof(ttl)) < 0)
    {
        i_warning("failed to set multicast ttl to "
                  "%u: %s\n", ttl, strerror(errno));
        setsockopt(engine->sync_fd, IPPROTO_IP,
            IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        goto fail;
    }

    i_debug("using multicast spam sync %smode "
            "(ttl %u, group %s, port %d)\n",
            engine->send_mcast ? "" : "receive ",
            ttl, inet_ntoa(engine->sync_out.sin_addr), engin->port);

    return engine->sync_fd;

 fail:
    close(engine->sync_fd);

    return -1;
}

extern void
Sync_stop(Sync_engine_T *engine)
{
    if((*engine)->sync_hosts)
        List_destroy(&(*engine)->sync_hosts);
    free(*engine);
    *engine = NULL;
}

extern int
Sync_add_host(Sync_engine_T engine, const char *name)
{
    struct addrinfo hints, *res, *next;
    struct sync_host *host;
    struct sockaddr_in *addr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(name, NULL, &hints, &next) != 0)
        return EINVAL;

    for(res = next; res != NULL; res = res->ai_next) {
        if(addr == NULL && res->ai_family == AF_INET) {
            addr = (struct sockaddr_in *) res->ai_addr;
            break;
        }
    }

    if(addr == NULL) {
        freeaddrinfo(next);
        return EINVAL;
    }

    if((host = calloc(1, sizeof(*host))) == NULL) {
        freeaddrinfo(next);
        return ENOMEM;
    }

    if((host->name = strdup(name)) == NULL) {
        free(host);
        freeaddrinfo(next);
        return ENOMEM;
    }

    host->addr.sin_family = AF_INET;
    host->addr.sin_port = htons(engine->port);
    host->addr.sin_addr.s_addr = addr->sin_addr.s_addr;
    freeaddrinfo(next);

    List_insert_after(engine->sync_hosts, host);
    i_debug("added spam sync host %s (address %s, port %d)",
            host->name, inet_ntoa(host->addr.sin_addr),
            engine->port);

    return 0;
}

extern void
Sync_recv(void)
{
}

extern void
Sync_update(time_t now, char *helo, char *ip, char *from, char *to)
{
}

extern void
Sync_white(time_t now, time_t expire, char *ip)
{
}

extern void
Sync_trapped(time_t now, time_t expire, char *ip)
{
}

static void
destroy_sync_host(void *entry)
{
    struct Sync_host *host = entry;

    if(host) {
        if(host->name) {
            free(host->name);
            host->name = NULL;
        }
        free(host);
    }
}
