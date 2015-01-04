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

/**
 * @file   sync.h
 * @brief  Defines the multicast/UDP sync engine interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef SYNC_DEFINED
#define SYNC_DEFINED

#include <netinet/in.h>
#include <openssl/sha.h>

#include "greyd_config.h"

#define SYNC_VERSION    1
#define SYNC_VERIFY_MSG 1
#define SYNC_MCASTADDR  "224.0.1.241"
#define SYNC_MCASTTTL   1
#define SYNC_HMAC_LEN   20  /* SHA1 */
#define SYNC_MAXSIZE    1408
#define SYNC_KEY        "/etc/greyd/greyd.key"

#define SYNC_END     0x0000
#define SYNC_GREY    0x0001
#define SYNC_WHITE   0x0002
#define SYNC_TRAPPED 0x0003

#define SYNC_ALIGNBYTES (15)
#define SYNC_ALIGN(p)   (((u_int)(p) + SYNC_ALIGNBYTES) &~ SYNC_ALIGNBYTES)

typedef struct Sync_engine_T *Sync_engine_T;
struct Sync_engine_T {
    Config_T config;
    u_short port;
    FILE *grey_out;
    int sync_fd;
    int send_mcast;
    struct sockaddr_in sync_in;
    struct sockaddr_in sync_out;
    unsigned char sync_key[SHA_DIGEST_LENGTH + 1];
    int sync_counter;
    List_T sync_hosts;
    char *iface;
};

struct Sync_hdr {
    u_int8_t  sh_version;
    u_int8_t  sh_af;
    u_int16_t sh_length;
    u_int32_t sh_counter;
    u_int8_t  sh_hmac[SYNC_HMAC_LEN];
    u_int8_t  sh_pad[4];
};

struct Sync_tlv_hdr {
    u_int16_t st_type;
    u_int16_t st_length;
};

struct Sync_tlv_grey {
    u_int16_t sg_type;
    u_int16_t sg_length;
    u_int32_t sg_timestamp;
    u_int32_t sg_ip;
    u_int16_t sg_from_length;
    u_int16_t sg_to_length;
    u_int16_t sg_helo_length;
    /* Strings go here, then packet code re-aligns packet. */
};

struct Sync_tlv_addr {
    u_int16_t sd_type;
    u_int16_t sd_length;
    u_int32_t sd_timestamp;
    u_int32_t sd_expire;
    u_int32_t sd_ip;
};

/**
 * Create a new engine object and initialize the engine's
 * fields based on the supplied configuration.
 */
extern Sync_engine_T Sync_init(Config_T config);

/**
 * Start the sync engine.
 *
 * @return the engine's socket file descriptor.
 */
extern int Sync_start(Sync_engine_T engine);

/**
 * Stop the sync engine and cleanup all sync resources.
 */
extern void Sync_stop(Sync_engine_T *engine);

/**
 * Add a new sync host to the engine's list of UDP sync hosts.
 */
extern int Sync_add_host(Sync_engine_T engine, const char *name);

/**
 * Receive a sync message on the engine's socket and write grey
 * data to the greylister on the specified file handle.
 */
extern void Sync_recv(Sync_engine_T engine, FILE *grey_out);

/**
 * Send out a sync message to notify others of a change to a grey
 * entry.
 */
extern void Sync_update(Sync_engine_T engine, struct Grey_tuple *gt,
                        time_t now);

/**
 * Send out a sync message to notify others of a change to a white
 * entry.
 */
extern void Sync_white(Sync_engine_T engine, char *ip, time_t now,
                       time_t expire);

/**
 * Send out a sync message to notify others of a change to a
 * grey-trapped entry.
 */
extern void Sync_trapped(Sync_engine_T engine, char *ip, time_t now,
                         time_t expire);

#endif
