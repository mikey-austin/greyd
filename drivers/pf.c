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

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef HAVE_NET_PF_IF_PFLOG_H
#include <net/pf/if_pflog.h>
#endif
#ifdef HAVE_NET_IF_PFLOG_H
#include <net/if_pflog.h>
#endif
#ifdef HAVE_NET_PF_PFVAR_H
#include <net/pf/pfvar.h>
#endif
#ifdef HAVE_NET_PFVAR_H
#include <net/pfvar.h>
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pcap.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/config_section.h"
#include "../src/constants.h"
#include "../src/failures.h"
#include "../src/firewall.h"
#include "../src/list.h"
#include "../src/utils.h"

#define PFDEV_PATH "/dev/pf"
#define PFLOG_IF "pflog0"
#define MAX_PFDEV 63

#define MIN_PFLOG_HDRLEN 45
#define PCAPSNAP 512
#define PCAPTIMO 500 /* ms */
#define PCAPOPTZ 1 /* optimize filter */
#define PCAPFSIZ 512 /* pcap filter string size */

#define satosin(sa) ((struct sockaddr_in*)(sa))
#define satosin6(sa) ((struct sockaddr_in6*)(sa))

struct fw_handle {
    int pfdev;
    pcap_t* pcap_handle;
    List_T entries;
};

/**
 * Setup a pipe for communication with the control command.
 */
static FILE* setup_cntl_pipe(char*, char**, int*);
static int server_lookup4(int, struct sockaddr_in*, struct sockaddr_in*,
    struct sockaddr_in*);
static int server_lookup6(int, struct sockaddr_in6*, struct sockaddr_in6*,
    struct sockaddr_in6*);
static void destroy_log_entry(void*);
static void packet_received(u_char*, const struct pcap_pkthdr*, const u_char*);

int Mod_fw_open(FW_handle_T handle)
{
    struct fw_handle* fwh = NULL;
    char* pfdev_path;
    int pfdev;

    pfdev_path = Config_get_str(handle->config, "pfdev_path",
        "firewall", PFDEV_PATH);

    if ((fwh = malloc(sizeof(*fwh))) == NULL)
        return -1;

    pfdev = open(pfdev_path, O_RDWR);
    if (pfdev < 1 || pfdev > MAX_PFDEV) {
        i_warning("could not open %s: %s", pfdev_path,
            strerror(errno));
        return -1;
    }

    fwh->pfdev = pfdev;
    fwh->pcap_handle = NULL;
    handle->fwh = fwh;

    return 0;
}

void Mod_fw_close(FW_handle_T handle)
{
    struct fw_handle* fwh = handle->fwh;

    if (fwh) {
        close(fwh->pfdev);
        free(fwh);
    }
    handle->fwh = NULL;
}

int Mod_fw_replace(FW_handle_T handle, const char* set_name, List_T cidrs, short af)
{
    struct fw_handle* fwh = handle->fwh;
    int fd, nadded = 0, child = -1, status;
    char *cidr, *fd_path = NULL, *pfctl_path = PFCTL_PATH;
    char* table = (char*)set_name;
    static FILE* pf = NULL;
    struct List_entry* entry;
    struct sigaction act, oldact;
    char* argv[11] = { "pfctl", "-p", PFDEV_PATH, "-q", "-t", table,
        "-T", "replace", "-f", "-", NULL };

    if (List_size(cidrs) == 0)
        return 0;

    pfctl_path = Config_get_str(handle->config, "pfctl_path",
        "firewall", PFCTL_PATH);

    if (asprintf(&fd_path, "/dev/fd/%d", fwh->pfdev) == -1)
        return -1;
    argv[2] = fd_path;

    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &act, &oldact);

    if ((pf = setup_cntl_pipe(pfctl_path, argv, &child)) == NULL) {
        free(fd_path);
        fd_path = NULL;
        goto err;
    }
    free(fd_path);
    fd_path = NULL;

    LIST_EACH(cidrs, entry)
    {
        if ((cidr = List_entry_value(entry)) != NULL) {
            fprintf(pf, "%s\n", cidr);
            nadded++;
        }
    }
    fclose(pf);

    waitpid(child, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        i_warning("%s returned status %d", pfctl_path,
            WEXITSTATUS(status));
        goto err;
    } else if (WIFSIGNALED(status)) {
        i_warning("%s died on signal %d", pfctl_path,
            WTERMSIG(status));
        goto err;
    }

    signal(SIGCHLD, &oldact, NULL);

    return nadded;

err:
    return -1;
}

int Mod_fw_lookup_orig_dst(FW_handle_T handle, struct sockaddr* src,
    struct sockaddr* proxy, struct sockaddr* orig_dst)
{
    struct fw_handle* fwh = handle->fwh;

    if (src->sa_family == AF_INET) {
        return server_lookup4(fwh->pfdev, satosin(src), satosin(proxy),
            satosin(orig_dst));
    }

    if (src->sa_family == AF_INET6) {
        return server_lookup6(fwh->pfdev, satosin6(src), satosin6(proxy),
            satosin6(orig_dst));
    }

    errno = EPROTONOSUPPORT;
    return -1;
}

void Mod_fw_start_log_capture(FW_handle_T handle)
{
    struct fw_handle* fwh = handle->fwh;
    struct bpf_program bpfp;
    char *pflog_if, *net_if;
    char errbuf[PCAP_ERRBUF_SIZE];
    char filter[PCAPFSIZ] = "ip and port 25 and action pass "
                            "and tcp[13]&0x12=0x2";

    pflog_if = Config_get_str(handle->config, "pflog_if", "firewall",
        PFLOG_IF);
    net_if = Config_get_str(handle->config, "net_if", "firewall",
        NULL);

    if ((fwh->pcap_handle = pcap_open_live(pflog_if, PCAPSNAP, 1, PCAPTIMO,
             errbuf))
        == NULL) {
        i_critical("failed to initialize: %s", errbuf);
    }

    if (pcap_datalink(fwh->pcap_handle) != DLT_PFLOG) {
        pcap_close(fwh->pcap_handle);
        fwh->pcap_handle = NULL;
        i_critical("invalid datalink type");
    }

    if (net_if != NULL) {
        sstrncat(filter, " and on ", PCAPFSIZ);
        sstrncat(filter, net_if, PCAPFSIZ);
    }

    if ((pcap_compile(fwh->pcap_handle, &bpfp, filter, PCAPOPTZ, 0) == -1)
        || (pcap_setfilter(fwh->pcap_handle, &bpfp) == -1)) {
        i_critical("%s", pcap_geterr(fwh->pcap_handle));
    }

    pcap_freecode(&bpfp);
#ifdef BIOCLOCK
    if (ioctl(pcap_fileno(fwh->pcap_handle), BIOCLOCK) < 0) {
        i_critical("BIOCLOCK: %s", strerror(errno));
    }
#endif

    fwh->entries = List_create(destroy_log_entry);
}

void Mod_fw_end_log_capture(FW_handle_T handle)
{
    struct fw_handle* fwh = handle->fwh;

    List_destroy(&fwh->entries);
    pcap_close(fwh->pcap_handle);
}

List_T
Mod_fw_capture_log(FW_handle_T handle)
{
    struct fw_handle* fwh = handle->fwh;
    pcap_handler ph = packet_received;

    List_remove_all(fwh->entries);
    pcap_dispatch(fwh->pcap_handle, 0, ph, (u_char*)handle);

    return fwh->entries;
}

static void
packet_received(u_char* args, const struct pcap_pkthdr* h, const u_char* sp)
{
    FW_handle_T handle = (FW_handle_T)args;
    struct fw_handle* fwh = handle->fwh;
    sa_family_t af;
    u_int8_t hdrlen;
    u_int32_t caplen = h->caplen;
    const struct ip* ip = NULL;
    const struct pfloghdr* hdr;
    char addr[INET6_ADDRSTRLEN] = { '\0' };
    int track_outbound;

    track_outbound = Config_get_int(handle->config, "track_outbound",
        "firewall", TRACK_OUTBOUND);

    hdr = (const struct pfloghdr*)sp;
    if (hdr->length < MIN_PFLOG_HDRLEN) {
        i_warning("invalid pflog header length (%u/%u). "
                  "packet dropped.",
            hdr->length, MIN_PFLOG_HDRLEN);
        return;
    }
    hdrlen = BPF_WORDALIGN(hdr->length);

    if (caplen < hdrlen) {
        i_warning("pflog header larger than caplen (%u/%u). "
                  "packet dropped.",
            hdrlen, caplen);
        return;
    }

    /* We're interested in passed packets */
    if (hdr->action != PF_PASS)
        return;

    af = hdr->af;
    if (af == AF_INET) {
        ip = (const struct ip*)(sp + hdrlen);
        if (hdr->dir == PF_IN) {
            inet_ntop(af, &ip->ip_src, addr,
                sizeof(addr));
        } else if (hdr->dir == PF_OUT && track_outbound) {
            inet_ntop(af, &ip->ip_dst, addr,
                sizeof(addr));
        }
    }

    if (addr[0] != '\0') {
        i_debug("packet received: direction = %s, addr = %s",
            (hdr->dir == PF_IN ? "in" : "out"), addr);
        List_insert_after(fwh->entries, strdup(addr));
    }
}

static int
server_lookup4(int pfdev, struct sockaddr_in* client, struct sockaddr_in* proxy,
    struct sockaddr_in* server)
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

    if (ioctl(pfdev, DIOCNATLOOK, &pnl) == -1)
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
server_lookup6(int pfdev, struct sockaddr_in6* client, struct sockaddr_in6* proxy,
    struct sockaddr_in6* server)
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

    if (ioctl(pfdev, DIOCNATLOOK, &pnl) == -1)
        return -1;

    memset(server, 0, sizeof(struct sockaddr_in6));
    server->sin6_len = sizeof(struct sockaddr_in6);
    server->sin6_family = AF_INET6;
    memcpy(&server->sin6_addr.s6_addr, &pnl.rdaddr.v6,
        sizeof(server->sin6_addr));
    server->sin6_port = pnl.rdport;

    return 0;
}

static FILE* setup_cntl_pipe(char* command, char** argv, int* pid)
{
    int pdes[2];
    FILE* out;

    if (pipe(pdes) != 0)
        return NULL;

    switch ((*pid = fork())) {
    case -1:
        close(pdes[0]);
        close(pdes[1]);
        return NULL;

    case 0:
        /* Close all output in child. */
        close(pdes[1]);
        close(STDERR_FILENO);
        close(STDOUT_FILENO);
        if (pdes[0] != STDIN_FILENO) {
            dup2(pdes[0], STDIN_FILENO);
            close(pdes[0]);
        }
        execvp(command, argv);
        _exit(1);
    }

    /* parent */
    close(pdes[0]);
    out = fdopen(pdes[1], "w");
    if (out == NULL) {
        close(pdes[1]);
        return NULL;
    }

    return out;
}

static void
destroy_log_entry(void* entry)
{
    if (entry != NULL)
        free(entry);
}
