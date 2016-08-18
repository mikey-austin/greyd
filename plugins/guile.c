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

#include <libguile.h>

#include "failures.h"
#include "plugin.h"
#include "config_section.h"

extern int load(Config_section_T);
extern void unload();

static void *register_functions(void *);
static int run_traps(struct Grey_tuple *, void *);

static SCM api_debug(SCM);
static SCM api_register_spamtrap(SCM);

static SCM Callback;

extern int
load(Config_section_T section)
{
    if(Config_section_get_int(section, "enable", 0)) {
        scm_with_guile(&register_functions, NULL);

        /* Load the actual plugin scheme code. */
        char *plugin = Config_section_get_str(
            section, "plugin_file", NULL);
        if(plugin)
            scm_c_primitive_load(plugin);

        /* Guile spamtrap runner. */
        Plugin_register_spamtrap("guile_traps", run_traps, NULL);
    }


    return PLUGIN_OK;
}

extern void
unload()
{
}

/*
 * Run the registered guile trap plugins with the grey tuple
 * as an argument.
 */
static int
run_traps(struct Grey_tuple *gt, void *arg)
{
    scm_apply_3(
        Callback,
        scm_from_locale_string(gt->ip),
        scm_from_locale_string(gt->helo),
        scm_from_locale_string(gt->from),
        //scm_from_locale_string(gt->to),
        SCM_EOL);
    return 0;
}

static void
*register_functions(void *args)
{
    scm_c_define_gsubr("register-spamtrap", 1, 0, 0, &api_register_spamtrap);
    scm_c_define_gsubr("debug", 1, 0, 0, &api_debug);

    return NULL;
}

static SCM
api_debug(SCM str)
{
    char *msg = scm_to_locale_string(str);
    i_debug(msg);
    free(msg);
    return SCM_UNSPECIFIED;
}

static SCM
api_register_spamtrap(SCM callback)
{
    Callback = callback;
    return SCM_UNSPECIFIED;
}
