/**
 * @file   ipset.c
 * @brief  Pluggable IPset firewall interface.
 * @author Mikey Austin
 * @date   2014
 *
 * This firewall driver makes use of libipset for the set management
 * and libnetfilter_conntrack for the DNAT original destination
 * lookups.
 */

#include "../failures.h"
#include "../firewall.h"
#include "../config_section.h"
#include "../list.h"
#include "../ip.h"
#include "../utils.h"

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
#include <libipset/types.h>
#include <libipset/session.h>
#include <libipset/data.h>

#define DEFAULT_SET     "greyd"
#define MAX_ELEM        200000
#define HASH_SIZE       (1024 * 1024)
#define IPV6_ADDR_PARTS 4  /* 4 32 bit parts. */
#define MAX_STAGE_NAME  256

struct cb_filter {
    struct sockaddr *src;
    struct sockaddr *proxy;
};

struct cb_data_arg {
    struct cb_filter filter;
    struct sockaddr *orig_dst;
};

struct fw_handle {
    struct mnl_socket *nl;
    struct ipset_session *session;
};

/**
 * libipset management convenience functions.
 */
static int ipset_create(struct ipset_session *, const char *, int, int, short);
static int ipset_add(struct ipset_session *, const char *, char *, short);
static int ipset_swap_and_destroy(struct ipset_session *, const char *, const char *);
static void set_effective_caps(void);

/**
 * This is the libnetfilter_conntrack data callback, called for each
 * conntrack object returned from the netlink socket. We need to do
 * filtering here as well to ensure the desired conntrack object is
 * used.
 */
static int cb_data(const struct nlmsghdr *, void *);

int
Mod_fw_open(FW_handle_T handle)
{
    struct fw_handle *fwh;
    cap_value_t cap_values[] = { CAP_NET_ADMIN };
    cap_t caps;

    if((fwh = malloc(sizeof(*fwh))) == NULL)
        err(1, "malloc");
    handle->fwh = fwh;

    fwh->nl = mnl_socket_open(NETLINK_NETFILTER);
    if(fwh->nl == NULL) {
        warn("mnl_socket_open");
    }
    else {
        if (mnl_socket_bind(fwh->nl, 0, MNL_SOCKET_AUTOPID) < 0)
            warn("mnl_socket_bind");
    }

    ipset_load_types();
    fwh->session = ipset_session_init(printf);
    ipset_envopt_parse(fwh->session, IPSET_ENV_EXIST, NULL);
    if(fwh->session == NULL)
        warn("ipset_session_init");

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
    struct fw_handle *fwh = handle->fwh;
    cap_t caps;

    if(fwh) {
        mnl_socket_close(fwh->nl);
        ipset_session_fini(fwh->session);
        free(fwh);
        handle->fwh = NULL;
    }

    /* Clear all capabilities upon closing the firewall handle. */
    caps = cap_get_proc();
    cap_clear(caps);
    cap_set_proc(caps);
    if(prctl(PR_SET_KEEPCAPS, 0, 0, 0, 0) == -1)
        warn("prctl");
    cap_free(caps);
}

int
Mod_fw_lookup_orig_dst(FW_handle_T handle, struct sockaddr *src,
                       struct sockaddr *proxy, struct sockaddr *orig_dst)
{
    struct mnl_socket *nl = ((struct fw_handle *) handle->fwh)->nl;
    struct cb_data_arg data;
    struct nlmsghdr *nlh;
    struct nfgenmsg *nfh;
    char buf[MNL_SOCKET_BUFFER_SIZE];
    unsigned int seq, portid;
    struct nf_conntrack *ct;
    sa_family_t af;
    int ret;

    set_effective_caps();

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
    else if(af == AF_INET6) {
        nfct_set_attr(ct, ATTR_REPL_IPV6_DST,
                      &((struct sockaddr_in6 *) src)->sin6_addr);
        nfct_set_attr(ct, ATTR_REPL_IPV6_SRC,
                      &((struct sockaddr_in6 *) proxy)->sin6_addr);
        nfct_set_attr_u16(ct, ATTR_PORT_SRC,
                          ((struct sockaddr_in6 *) src)->sin6_port);
        nfct_set_attr_u16(ct, ATTR_REPL_PORT_SRC,
                          ((struct sockaddr_in6 *) proxy)->sin6_port);
    }

    nfct_nlmsg_build(nlh, ct);

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
        i_warning("mnl_socket_recvfrom");

    return 0;
}

static int
cb_data(const struct nlmsghdr *nlh, void *arg)
{
    struct cb_data_arg *data = (struct cb_data_arg *) arg;
    struct nf_conntrack *ct;
    u_int32_t ct_src[IPV6_ADDR_PARTS], ct_proxy[IPV6_ADDR_PARTS];
    u_int32_t ct_orig_dst[IPV6_ADDR_PARTS];
    u_int32_t *cti32, *pp, *sp;
    u_int16_t src_port, reply_src_port;
    u_int16_t ct_src_port, ct_reply_src_port;
    sa_family_t af;
    int i;

    ct = nfct_new();
    if(ct == NULL)
        return MNL_CB_OK;

    nfct_nlmsg_parse(nlh, ct);

    af = data->filter.src->sa_family;
    if(af == AF_INET) {
        src_port = ((struct sockaddr_in *) data->filter.src)->sin_port;
        reply_src_port = ((struct sockaddr_in *) data->filter.proxy)->sin_port;

        *ct_src = nfct_get_attr_u32(ct, ATTR_REPL_IPV4_DST);
        *ct_proxy = nfct_get_attr_u32(ct, ATTR_REPL_IPV4_SRC);
        ct_src_port = nfct_get_attr_u16(ct, ATTR_PORT_SRC);
        ct_reply_src_port = nfct_get_attr_u16(ct, ATTR_REPL_PORT_SRC);

        if((ct_reply_src_port == reply_src_port)
           && (ct_src_port == src_port)
           && (*ct_proxy == ((struct sockaddr_in *) data->filter.proxy)->sin_addr.s_addr)
           && (*ct_src == ((struct sockaddr_in *) data->filter.src)->sin_addr.s_addr))
        {
            /* This is the conntrack object we are after. */
            *ct_orig_dst = nfct_get_attr_u32(ct, ATTR_IPV4_DST);
            memset(data->orig_dst, 0, sizeof(*((struct sockaddr_in *) data->orig_dst)));
            data->orig_dst->sa_family = af;
            ((struct sockaddr_in *) data->orig_dst)->sin_port = reply_src_port;
            ((struct sockaddr_in *) data->orig_dst)->sin_addr.s_addr = *ct_orig_dst;
        }
    }
    else if(af == AF_INET6) {
        src_port = ((struct sockaddr_in6 *) data->filter.src)->sin6_port;
        reply_src_port = ((struct sockaddr_in6 *) data->filter.proxy)->sin6_port;

        cti32 = (u_int32_t *) nfct_get_attr(ct, ATTR_REPL_IPV6_DST);
        if(cti32 != NULL)
            for(i = 0; i < 4; i++)
                ct_src[i] = cti32[i];

        cti32 = (u_int32_t *) nfct_get_attr(ct, ATTR_REPL_IPV6_SRC);
        if(cti32 != NULL)
            for(i = 0; i < IPV6_ADDR_PARTS; i++)
                ct_proxy[i] = cti32[i];

        ct_src_port = nfct_get_attr_u16(ct, ATTR_PORT_SRC);
        ct_reply_src_port = nfct_get_attr_u16(ct, ATTR_REPL_PORT_SRC);

        pp = (u_int32_t *) &((struct sockaddr_in6 *) data->filter.proxy)->sin6_addr.s6_addr;
        sp = (u_int32_t *) &((struct sockaddr_in6 *) data->filter.src)->sin6_addr.s6_addr;

        if((ct_reply_src_port == reply_src_port)
           && (ct_src_port == src_port)
           && (ct_proxy[0] == pp[0] && ct_proxy[1] == pp[1]
               && ct_proxy[2] == pp[2] && ct_proxy[3] == pp[3])
           && (ct_src[0] == sp[0] && ct_src[1] == sp[1]
               && ct_src[2] == sp[2] && ct_src[3] == sp[3])
           && (cti32 = (u_int32_t *) nfct_get_attr(ct, ATTR_IPV6_DST)))
        {
            /* This is the conntrack object we are after. */
            memset(data->orig_dst, 0, sizeof(*((struct sockaddr_in6 *) data->orig_dst)));
            data->orig_dst->sa_family = af;
            ((struct sockaddr_in6 *) data->orig_dst)->sin6_port = reply_src_port;
            pp = (u_int32_t *) &((struct sockaddr_in6 *) data->orig_dst)->sin6_addr.s6_addr;
            for(i = 0; i < IPV6_ADDR_PARTS; i++)
                pp[i] = cti32[i];
        }
    }

    nfct_destroy(ct);

    return MNL_CB_OK;
}

int
Mod_fw_replace(FW_handle_T handle, const char *set_name, List_T cidrs, short af)
{
    struct ipset_session *session = ((struct fw_handle *) handle->fwh)->session;
    struct List_entry *entry;
    char *cidr, stage_set_name[MAX_STAGE_NAME];
    int nadded = 0, hash_size, max_elem;

    sstrncpy(stage_set_name, set_name, sizeof(stage_set_name));
    sstrncat(stage_set_name, "-stage", sizeof(stage_set_name));

    set_effective_caps();

    if(session == NULL)
        return -1;

    hash_size = Config_section_get_int(
        handle->section, "hash_size", HASH_SIZE);
    max_elem = Config_section_get_int(
        handle->section, "max_elements", MAX_ELEM);

    if(ipset_create(session, stage_set_name, hash_size, max_elem, af) == -1)
        return -1;

    LIST_FOREACH(cidrs, entry) {
        if((cidr = List_entry_value(entry)) != NULL) {
            if(ipset_add(session, stage_set_name, cidr, af) == -1) {
                warnx("invalid cidr %s", cidr);
                return -1;
            }
            nadded++;
        }
    }

    if(ipset_create(session, set_name, hash_size, max_elem, af) == -1)
        return -1;

    if(ipset_swap_and_destroy(session, set_name, stage_set_name) == -1)
        return -1;

    return nadded;
}

static int
ipset_create(struct ipset_session *session, const char *set_name,
             int hash_size, int max_elem, short af)
{
    u_int8_t family;
    const struct ipset_type *type;
    u_int32_t timeout = 0;   /* Unlimited. */

    if(ipset_session_data_set(session, IPSET_SETNAME, set_name) != 0) {
        warnx("ipset create %s: %s", set_name,
              ipset_session_error(session));
        return -1;
    }

    ipset_session_data_set(session, IPSET_OPT_TYPENAME, "hash:net");

    if((type = ipset_type_get(session, IPSET_CMD_CREATE)) == NULL) {
        warnx("ipset type get %s: %s", set_name,
              ipset_session_error(session));
        return -1;
    }
    ipset_session_data_set(session, IPSET_OPT_TYPE, type);

    family = (af == AF_INET6 ? NFPROTO_IPV6 : NFPROTO_IPV4);
    ipset_session_data_set(session, IPSET_OPT_FAMILY, &family);
    ipset_session_data_set(session, IPSET_OPT_MAXELEM, &max_elem);
    ipset_session_data_set(session, IPSET_OPT_HASHSIZE, &hash_size);
    ipset_session_data_set(session, IPSET_OPT_EXIST, NULL);
    ipset_session_data_set(session, IPSET_OPT_TIMEOUT, &timeout);

    if(ipset_cmd(session, IPSET_CMD_CREATE, 0) == -1) {
        warnx("ipset create %s: %s", set_name,
              ipset_session_error(session));
        return -1;
    }

    return 0;
}

static int
ipset_add(struct ipset_session *session, const char *set_name, char *cidr,
          short af)
{
    u_int8_t family;
    unsigned int maskbits = 0;
    char parsed[INET6_ADDRSTRLEN];
    const struct ipset_type *type;
    struct sockaddr_storage addr;

    if(sscanf(cidr, "%39[^/]/%u", parsed, &maskbits) < 1
       || maskbits > IP_MAX_MASKBITS)
    {
        return -1;
    }

    if(maskbits == 0)
        maskbits = (af == AF_INET6 ? 128 : 32);

    if(ipset_session_data_set(session, IPSET_SETNAME, set_name) != 0)
        return -1;

    if((type = ipset_type_get(session, IPSET_CMD_ADD)) == NULL)
        return -1;

    family = (af == AF_INET6 ? NFPROTO_IPV6 : NFPROTO_IPV4);
    ipset_session_data_set(session, IPSET_OPT_FAMILY, &family);
    ipset_session_data_set(session, IPSET_OPT_CIDR, &maskbits);
    ipset_session_data_set(session, IPSET_OPT_EXIST, NULL);

    memset(&addr, 0, sizeof(addr));
    if(af == AF_INET6) {
        if(inet_pton(af, parsed, &((struct sockaddr_in6 *) &addr)->sin6_addr) == -1)
            return -1;
        ipset_session_data_set(session, IPSET_OPT_IP,
                               &((struct sockaddr_in6 *) &addr)->sin6_addr);
    }
    else {
        if(inet_pton(af, parsed, &((struct sockaddr_in *) &addr)->sin_addr) == -1)
            return -1;
        ipset_session_data_set(session, IPSET_OPT_IP,
                               &((struct sockaddr_in *) &addr)->sin_addr);
    }

    if(ipset_cmd(session, IPSET_CMD_ADD, 0) == -1) {
        warnx("ipset add %s: %s", set_name,
              ipset_session_error(session));
        return -1;
    }

    return 0;
}

static int
ipset_swap_and_destroy(struct ipset_session *session, const char *set_name,
                       const char *stage_set_name)
{
    if(ipset_session_data_set(session, IPSET_SETNAME, set_name) != 0)
        return -1;

    if(ipset_session_data_set(session, IPSET_OPT_SETNAME2, stage_set_name) != 0)
        return -1;

    if(ipset_cmd(session, IPSET_CMD_SWAP, 0) != 0) {
        warnx("ipset swap %s <-> %s: %s", set_name, stage_set_name,
              ipset_session_error(session));
        return -1;
    }

    /*
     * As we just swapped, the stage set is under the old name, so
     * deleting the set under the stage set's name is what we want.
     */
    if(ipset_session_data_set(session, IPSET_SETNAME, stage_set_name) != 0)
        return -1;

    if(ipset_cmd(session, IPSET_CMD_DESTROY, 0) == -1) {
        warnx("ipset destroy %s: %s", stage_set_name,
              ipset_session_error(session));
        return -1;
    }

    return 0;
}

static void
set_effective_caps()
{
    cap_value_t cap_values[] = { CAP_NET_ADMIN };
    cap_t caps;

    /* Set the effective capabilities. */
    caps = cap_get_proc();
    cap_clear(caps);
    if(cap_set_flag(caps, CAP_PERMITTED, 1, cap_values, CAP_SET) == -1)
        warn("cap_set_flag");
    if(cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_values, CAP_SET) == -1)
        warn("cap_set_flag");
    cap_set_proc(caps);
    cap_free(caps);
}
