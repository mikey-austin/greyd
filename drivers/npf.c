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
 * @file   npf.c
 * @brief  Pluggable NPF firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include <config.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <net/npf.h>
#include <npf.h>
#include <pcap.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/config_section.h"
#include "../src/constants.h"
#include "../src/failures.h"
#include "../src/firewall.h"
#include "../src/ip.h"
#include "../src/list.h"
#include "../src/utils.h"

#define NPFDEV_PATH "/dev/npf"
#define NPFLOG_IF "npflog0"
#define TABLE_ID 5

#define PCAPSNAP 512
#define PCAPTIMO 500 /* ms */
#define PCAPOPTZ 1 /* optimize filter */
#define PCAPFSIZ 512 /* pcap filter string size */

#define satosin(sa) ((struct sockaddr_in*)(sa))
#define satosin6(sa) ((struct sockaddr_in6*)(sa))

struct fw_handle {
    int npfdev;
    pcap_t* pcap_handle;
    List_T entries;
};

/**
 * Setup a pipe for communication with the control command.
 */
static void destroy_log_entry(void*);
static void packet_received(u_char*, const struct pcap_pkthdr*, const u_char*);

int Mod_fw_open(FW_handle_T handle)
{
    struct fw_handle* fwh = NULL;
    char* npfdev_path;
    int npfdev;

    npfdev_path = Config_get_str(handle->config, "npfdev_path",
        "firewall", NPFDEV_PATH);

    if ((fwh = malloc(sizeof(*fwh))) == NULL)
        return -1;

    npfdev = open(npfdev_path, O_RDONLY);
    if (npfdev < 1) {
        i_warning("could not open %s: %s", npfdev_path,
            strerror(errno));
        return -1;
    }

    fwh->npfdev = npfdev;
    fwh->pcap_handle = NULL;
    handle->fwh = fwh;

    return 0;
}

void Mod_fw_close(FW_handle_T handle)
{
    struct fw_handle* fwh = handle->fwh;

    if (fwh) {
        close(fwh->npfdev);
        free(fwh);
    }
    handle->fwh = NULL;
}

int Mod_fw_replace(FW_handle_T handle, const char* set_name, List_T cidrs, short af)
{
    struct fw_handle* fwh = handle->fwh;
    int fd, nadded = 0;
    char *cidr, *fd_path = NULL;
    char* table = (char*)set_name;
    void* handler;
    struct List_entry* entry;
    nl_config_t* ncf;
    nl_table_t* nt;
    struct IP_addr m, n;
    int ret;
    uint8_t maskbits;
    char parsed[INET6_ADDRSTRLEN];

    if (List_size(cidrs) == 0)
        return 0;

    ncf = npf_config_create();
    nt = npf_table_create(TABLE_ID, NPF_TABLE_HASH);

    /* This should somehow be atomic. */
    LIST_EACH(cidrs, entry)
    {
        if ((cidr = List_entry_value(entry)) != NULL
            && IP_str_to_addr_mask(cidr, &n, &m) != -1) {
            ret = sscanf(cidr, "%39[^/]/%u", parsed, &maskbits);
            if (ret != 2 || maskbits == 0 || maskbits > IP_MAX_MASKBITS)
                continue;

            npf_table_add_entry(nt, af, (npf_addr_t*)&n, *((npf_netmask_t*)&maskbits));
            nadded++;
        }
    }

    npf_table_insert(ncf, nt);
    npf_config_submit(ncf, fwh->npfdev);
    npf_config_destroy(ncf);
    npf_table_destroy(nt);
    nt = NULL;
    ncf = NULL;

    return nadded;

err:
    return -1;
}

int Mod_fw_lookup_orig_dst(FW_handle_T handle, struct sockaddr* src,
    struct sockaddr* proxy, struct sockaddr* orig_dst)
{
    return -1;
}

void Mod_fw_start_log_capture(FW_handle_T handle)
{
}

void Mod_fw_end_log_capture(FW_handle_T handle)
{
}

List_T
Mod_fw_capture_log(FW_handle_T handle)
{
    return NULL;
}

static void
packet_received(u_char* args, const struct pcap_pkthdr* h, const u_char* sp)
{
}

static void
destroy_log_entry(void* entry)
{
    if (entry != NULL)
        free(entry);
}
