/*
 * Copyright (c) 2016 Mikey Austin <mikey@greyd.org>
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

#ifdef HAVE_SPF
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <net/if.h>
#endif
#ifdef HAVE_SPF2_SPF_H
#  include <spf2/spf.h>
#endif
#ifdef HAVE_SPF_H
#  include <spf.h>
#endif

#include "constants.h"
#include "plugin.h"
#include "failures.h"
#include "grey.h"
#include "config_section.h"

/* One spf server per lifetime of plugin. */
static SPF_server_t *spf_server = NULL;
static Config_section_T Plugin_section;

static int spf_check(struct Grey_tuple *, void *);
extern void load(Config_section_T);
extern void unload();

static int
spf_check(struct Grey_tuple *gt, void *arg)
{
    SPF_request_t *req;
    SPF_response_t *res = NULL;
    int result = -1;

    if(spf_server == NULL)
        return 1;

    req = SPF_request_new(spf_server);
    if(SPF_request_set_ipv4_str(req, gt->ip))
        goto error;

    if(SPF_request_set_env_from(req, gt->from))
        goto error;

    if(SPF_request_set_helo_dom(req, gt->helo))
        goto error;

    SPF_request_query_mailfrom(req, &res);
    switch(SPF_response_result(res))
    {
    case SPF_RESULT_PASS:
        result = 1;
        i_info("SPF passed for %s %s helo %s",
               gt->ip, gt->from, gt->helo);
        break;

    case SPF_RESULT_NEUTRAL:
    case SPF_RESULT_NONE:
        result = 1;
        break;

    case SPF_RESULT_SOFTFAIL:
        if(!Config_section_get_int(
            Plugin_section, "trap_on_softfail", SPF_TRAP_SOFTFAIL))
        {
            result = 1;
            break;
        }
        /* Fallthrough. */

    case SPF_RESULT_FAIL:
        result = 1;
        i_warning("SPF failure for %s %s helo %s",
                  gt->ip, gt->from, gt->helo);
        break;

    default:
        i_warning("SPF error for %s %s helo %s: %s (%d)",
                  gt->ip, gt->from, gt->helo,
                  SPF_strerror(SPF_response_errcode(res)),
                  SPF_response_errcode(res));
        break;
    }

error:
    if(res)
        SPF_response_free(res);
    SPF_request_free(req);

    return result;
}

extern int
load(Config_section_T section)
{
    Plugin_section = section;
    int status = PLUGIN_OK;

    if(Config_section_get_int(section, "enable", SPF_ENABLED)) {
        spf_server = SPF_server_new(SPF_DNS_CACHE, 1);
        if(spf_server == NULL) {
            i_critical("could not create SPF server");
            status = PLUGIN_ERR;
            goto cleanup;
        }
    }

cleanup:
    return status;
}

extern void
unload()
{
    if(spf_server) {
        SPF_server_free(spf_server);
        spf_server = NULL;
    }
}
