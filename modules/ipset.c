/**
 * @file   ipset.c
 * @brief  Pluggable IPset firewall interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "../firewall.h"
#include "../config_section.h"
#include "../list.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <err.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/prctl.h>
#include <sys/capability.h>

#include <libmnl/libmnl.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#include <linux/netfilter/nf_conntrack_tcp.h>

#define DEFAULT_SET "greyd"
#define MAX_ELEM    200000
#define HASH_SIZE   (1024 * 1024)

struct cb_filter {
    struct sockaddr *src;
    struct sockaddr *proxy;
};

struct cb_data_arg {
    struct cb_filter filter;
    struct sockaddr *orig_dst;
};

static int cb_data(const struct nlmsghdr *nlh, void *);

int
Mod_fw_open(FW_handle_T handle)
{
	struct mnl_socket *nl;
    cap_value_t cap_values[] = { CAP_NET_ADMIN };
    cap_t caps;

	nl = mnl_socket_open(NETLINK_NETFILTER);
	if(nl == NULL) {
		warn("mnl_socket_open");
    }
    else {
        if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0)
            warn("mnl_socket_bind");
    }

    /*
     * Store the netlink socket in the handle.
     */
    handle->fwh = nl;

    /*
     * Try to keep capabilities so that the above socket can
     * be used after privileges are dropped.
     */
    caps = cap_get_proc();
    cap_set_flag(caps, CAP_PERMITTED, 1, cap_values, CAP_SET);
    cap_set_proc(caps);
    if(prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == -1)
        warn("prctl");
    cap_free(caps);

    return 0;
}

void
Mod_fw_close(FW_handle_T handle)
{
    struct mnl_socket *nl = handle->fwh;

    if(nl) {
        mnl_socket_close(nl);
        handle->fwh = NULL;
    }
}

int
Mod_fw_replace(FW_handle_T handle, const char *set_name, List_T cidrs)
{
    FILE *ipset = handle->fwh;
    struct List_entry *entry;
    char *cidr;
    int nadded = 0, hash_size, max_elem;

    hash_size = Config_section_get_int(
        handle->section, "hash_size", HASH_SIZE);
    max_elem = Config_section_get_int(
        handle->section, "max_elements", MAX_ELEM);

    /*
     * Prepare a new temporary set to populate, then replace the
     * existing set of the specified name with the new set, deleting
     * the old.
     */
    fprintf(ipset, "create temp-%s hash:net hashsize %d maxelem %d -exist\n",
            set_name, hash_size, max_elem);
    fprintf(ipset, "flush temp-%s\n", set_name);

    /* Add the netblocks to the temporary set. */
    LIST_FOREACH(cidrs, entry) {
        if((cidr = List_entry_value(entry)) != NULL) {
            fprintf(ipset, "add temp-%s %s -exist\n", set_name, cidr);
            nadded++;
        }
    }

    /* Make sure the desired list exists before swapping. */
    fprintf(ipset, "create %s hash:net hashsize %d maxelem %d -exist\n",
            set_name, hash_size, max_elem);

    fprintf(ipset, "swap %s temp-%s\n", set_name, set_name);
    fprintf(ipset, "destroy temp-%s\n", set_name);

    return nadded;
}

int
Mod_fw_lookup_orig_dst(FW_handle_T handle, struct sockaddr *src,
                       struct sockaddr *proxy, struct sockaddr *orig_dst)
{
    struct mnl_socket *nl = handle->fwh;
    struct cb_data_arg data;
	struct nlmsghdr *nlh;
	struct nfgenmsg *nfh;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	unsigned int seq, portid;
	struct nf_conntrack *ct;
    sa_family_t af;
	int ret;
    cap_value_t cap_values[] = { CAP_NET_ADMIN };
    cap_t caps;

    if(nl == NULL)
        return 0;

    data.filter.src = src;
    data.filter.proxy = proxy;
    data.orig_dst = orig_dst;

    /* Default to the proxy address. */
    memset(data.orig_dst, 0, sizeof(*data.orig_dst));
    memcpy(data.orig_dst, proxy, sizeof(*data.orig_dst));

    af = data.filter.src->sa_family;
	portid = mnl_socket_get_portid(nl);

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type = (NFNL_SUBSYS_CTNETLINK << 8) | IPCTNL_MSG_CT_GET;
	nlh->nlmsg_flags = NLM_F_REQUEST|NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);

	nfh = mnl_nlmsg_put_extra_header(nlh, sizeof(struct nfgenmsg));
	nfh->nfgen_family = af;
	nfh->version = NFNETLINK_V0;
	nfh->res_id = 0;

	ct = nfct_new();
	if(ct == NULL) {
        warn("nfct_new");
		return 0;
	}

	nfct_set_attr_u8(ct, ATTR_L3PROTO, af);
	nfct_set_attr_u8(ct, ATTR_L4PROTO, IPPROTO_TCP);
    if(af == AF_INET) {
        nfct_set_attr_u32(ct, ATTR_REPL_IPV4_DST,
                          ((struct sockaddr_in *) src)->sin_addr.s_addr);
        nfct_set_attr_u32(ct, ATTR_REPL_IPV4_SRC,
                          ((struct sockaddr_in *) proxy)->sin_addr.s_addr);
        nfct_set_attr_u16(ct, ATTR_PORT_SRC,
                          ((struct sockaddr_in *) src)->sin_port);
        nfct_set_attr_u16(ct, ATTR_REPL_PORT_SRC,
                          ((struct sockaddr_in *) proxy)->sin_port);
    }

	nfct_nlmsg_build(nlh, ct);

    /* Set the effective capabilities. */
    caps = cap_get_proc();
    if(cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_values, CAP_SET) == -1)
        warn("cap_set_flag");
    cap_set_proc(caps);
    cap_free(caps);

	ret = mnl_socket_sendto(nl, nlh, nlh->nlmsg_len);
	if(ret == -1)
		warn("mnl_socket_sendto");

    ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while(ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, cb_data, &data);
		if(ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}

	if(ret == -1)
		I_WARN("mnl_socket_recvfrom");

    return 0;
}

static int
cb_data(const struct nlmsghdr *nlh, void *arg)
{
    struct cb_data_arg *data = (struct cb_data_arg *) arg;
	struct nf_conntrack *ct;
    u_int32_t ct_src, ct_proxy, ct_orig_dst;
    u_int16_t src_port, reply_src_port;
    u_int16_t ct_src_port, ct_reply_src_port;
    sa_family_t af;

	ct = nfct_new();
	if(ct == NULL)
		return MNL_CB_OK;

	nfct_nlmsg_parse(nlh, ct);

    af = data->filter.src->sa_family;
    if(af == AF_INET) {
        src_port = ((struct sockaddr_in *) data->filter.src)->sin_port;
        reply_src_port = ((struct sockaddr_in *) data->filter.proxy)->sin_port;

        ct_src = nfct_get_attr_u32(ct, ATTR_REPL_IPV4_DST);
        ct_proxy = nfct_get_attr_u32(ct, ATTR_REPL_IPV4_SRC);
        ct_src_port = nfct_get_attr_u16(ct, ATTR_PORT_SRC);
        ct_reply_src_port = nfct_get_attr_u16(ct, ATTR_REPL_PORT_SRC);

        if((ct_reply_src_port == reply_src_port)
           && (ct_src_port == src_port)
           && (ct_proxy == ((struct sockaddr_in *) data->filter.proxy)->sin_addr.s_addr)
           && (ct_src == ((struct sockaddr_in *) data->filter.src)->sin_addr.s_addr))
        {
            /* We have a match. */
            ct_orig_dst = nfct_get_attr_u32(ct, ATTR_IPV4_DST);
            memset(data->orig_dst, 0, sizeof(*data->orig_dst));
            data->orig_dst->sa_family = af;
            ((struct sockaddr_in *) data->orig_dst)->sin_port = reply_src_port;
            ((struct sockaddr_in *) data->orig_dst)->sin_addr.s_addr = ct_orig_dst;
        }
    }

	nfct_destroy(ct);

	return MNL_CB_OK;
}
