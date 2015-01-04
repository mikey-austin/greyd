/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>
 * Copyright (c) 2004-2006 Bob Beck.  All rights reserved.
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
 * @file   pf.c
 * @brief  Pluggable PF firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "../src/failures.h"
#include "../src/firewall.h"
#include "../src/config_section.h"
#include "../src/list.h"
#include "../src/ip.h"

#define PFCTL_PATH "/sbin/pfctl"
#define PFDEV_PATH "/dev/pf"
#define MAX_PFDEV  63

#define satosin(sa)  ((struct sockaddr_in *)(sa))
#define satosin6(sa) ((struct sockaddr_in6 *)(sa))

/**
 * Setup a pipe for communication with the control command.
 */
static FILE *setup_cntl_pipe(char *, char **, int *);
static int server_lookup4(int, struct sockaddr_in *, struct sockaddr_in *,
                          struct sockaddr_in *);
static int server_lookup6(int, struct sockaddr_in6 *, struct sockaddr_in6 *,
                          struct sockaddr_in6 *);

int
Mod_fw_open(FW_handle_T handle)
{
    char *pfdev_path;
    int *pfdev = NULL;

    pfdev_path = Config_get_str(handle->config, "pfdev_path",
                                "firewall", PFDEV_PATH);

    if((pfdev = malloc(sizeof(*pfdev))) == NULL)
        return -1;

    *pfdev = open(pfdev_path, O_RDWR);
    if(*pfdev < 1 || *pfdev > MAX_PFDEV) {
        i_warning("could not open %s: %s", pfdev_path,
                  strerror(errno));
        return -1;
    }

    handle->fwh = pfdev;

    return 0;
}

void
Mod_fw_close(FW_handle_T handle)
{
    int *pfdev = handle->fwh;

    if(pfdev != NULL) {
        close(*pfdev);
        free(pfdev);
    }
    handle->fwh = NULL;
}

int
Mod_fw_replace(FW_handle_T handle, const char *set_name, List_T cidrs, short af)
{
    int fd, nadded = 0, child = -1, status, *pfdev;
    char *cidr, *fd_path = NULL, *pfctl_path = PFCTL_PATH;
    char *table = (char *) set_name;
    static FILE *pf = NULL;
    struct List_entry *entry;
    char *argv[11] = { "pfctl", "-p", PFDEV_PATH, "-q", "-t", table,
                       "-T", "replace", "-f", "-", NULL };

    pfctl_path = Config_get_str(handle->config, "pfctl_path",
                                "firewall", PFCTL_PATH);

    pfdev = handle->fwh;
    if(asprintf(&fd_path, "/dev/fd/%d", *pfdev) == -1)
        return -1;
    argv[2] = fd_path;

    if((pf = setup_cntl_pipe(pfctl_path, argv, &child)) == NULL) {
        free(fd_path);
        fd_path = NULL;
        goto err;
    }
    free(fd_path);
    fd_path = NULL;

    LIST_FOREACH(cidrs, entry) {
        if((cidr = List_entry_value(entry)) != NULL) {
            fprintf(pf, "%s\n", cidr);
            nadded++;
        }
    }

    waitpid(child, &status, 0);
    if(WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        i_warning("%s returned status %d", pfctl_path,
                  WEXITSTATUS(status));
        goto err;
    }
    else if(WIFSIGNALED(status)) {
        i_warning("%s died on signal %d", pfctl_path,
                  WTERMSIG(status));
        goto err;
    }

    return nadded;

err:
    return -1;
}

int
Mod_fw_lookup_orig_dst(FW_handle_T handle, struct sockaddr *src,
                       struct sockaddr *proxy, struct sockaddr *orig_dst)
{
    int *pfdev = handle->fwh;

    if(src->sa_family == AF_INET) {
        return server_lookup4(*pfdev, satosin(src), satosin(proxy),
                              satosin(orig_dst));
    }

    if(src->sa_family == AF_INET6) {
        return server_lookup6(*pfdev, satosin6(src), satosin6(proxy),
                              satosin6(orig_dst));
    }

    errno = EPROTONOSUPPORT;
    return -1;
}

static int
server_lookup4(int pfdev, struct sockaddr_in *client, struct sockaddr_in *proxy,
               struct sockaddr_in *server)
{
    struct pfioc_natlook pnl;

    memset(&pnl, 0, sizeof pnl);
    pnl.direction = PF_OUT;
    pnl.af = AF_INET;
    pnl.proto = IPPROTO_TCP;
    memcpy(&pnl.saddr.v4, &client->sin_addr.s_addr, sizeof pnl.saddr.v4);
    memcpy(&pnl.daddr.v4, &proxy->sin_addr.s_addr, sizeof pnl.daddr.v4);
    pnl.sport = client->sin_port;
    pnl.dport = proxy->sin_port;

    if(ioctl(pfdev, DIOCNATLOOK, &pnl) == -1)
        return -1;

    memset(server, 0, sizeof(struct sockaddr_in));
    server->sin_len = sizeof(struct sockaddr_in);
    server->sin_family = AF_INET;
    memcpy(&server->sin_addr.s_addr, &pnl.rdaddr.v4,
           sizeof(server->sin_addr.s_addr));
    server->sin_port = pnl.rdport;

    return 0;
}

static int
server_lookup6(int pfdev, struct sockaddr_in6 *client, struct sockaddr_in6 *proxy,
               struct sockaddr_in6 *server)
{
    struct pfioc_natlook pnl;

    memset(&pnl, 0, sizeof pnl);
    pnl.direction = PF_OUT;
    pnl.af = AF_INET6;
    pnl.proto = IPPROTO_TCP;
    memcpy(&pnl.saddr.v6, &client->sin6_addr.s6_addr, sizeof pnl.saddr.v6);
    memcpy(&pnl.daddr.v6, &proxy->sin6_addr.s6_addr, sizeof pnl.daddr.v6);
    pnl.sport = client->sin6_port;
    pnl.dport = proxy->sin6_port;

    if(ioctl(pfdev, DIOCNATLOOK, &pnl) == -1)
        return -1;

    memset(server, 0, sizeof(struct sockaddr_in6));
    server->sin6_len = sizeof(struct sockaddr_in6);
    server->sin6_family = AF_INET6;
    memcpy(&server->sin6_addr.s6_addr, &pnl.rdaddr.v6,
           sizeof(server->sin6_addr));
    server->sin6_port = pnl.rdport;

    return 0;
}

static FILE
*setup_cntl_pipe(char *command, char **argv, int *pid)
{
    int pdes[2];
    FILE *out;

    if(pipe(pdes) != 0)
        return NULL;

    switch((*pid = fork())) {
    case -1:
        close(pdes[0]);
        close(pdes[1]);
        return NULL;

    case 0:
        /* Close all output in child. */
        close(pdes[1]);
        close(STDERR_FILENO);
        close(STDOUT_FILENO);
        if(pdes[0] != STDIN_FILENO) {
            dup2(pdes[0], STDIN_FILENO);
            close(pdes[0]);
        }
        execvp(command, argv);
        _exit(1);
    }

    /* parent */
    close(pdes[0]);
    out = fdopen(pdes[1], "w");
    if(out == NULL) {
        close(pdes[1]);
        return NULL;
    }

    return out;
}
