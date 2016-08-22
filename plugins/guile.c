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
#include <glob.h>

#include "failures.h"
#include "plugin.h"
#include "list.h"
#include "config_section.h"

struct trap_data {
    struct Grey_tuple *gt;
    SCM trap;
};

extern int load(Config_section_T);
extern void unload();

static int guile_spamtrap(struct Grey_tuple *, void *);
static SCM spamtrap_try(void *);
static SCM spamtrap_catch(void *, SCM, SCM);

/* C to guile api functions. */
static void *register_api(void *);
static SCM api_register_spamtrap(SCM, SCM);
static SCM api_debug(SCM);
static SCM api_warn(SCM);
static SCM api_info(SCM);

extern int
load(Config_section_T section)
{
    List_T scripts;
    struct List_entry *entry = NULL;
    char *path = NULL;
    glob_t files;
    Config_value_T val;
    int i;

    if(Config_section_get_int(section, "enable", 0)) {
        scripts = Config_section_get_list(section, "scripts");
        if(scripts && List_size(scripts) > 0) {
            /* Only load guile if there are scripts. */
            scm_with_guile(&register_api, NULL);

            /* Treat each entry as a glob pattern. */
            LIST_EACH(scripts, entry) {
                val = List_entry_value(entry);
                if((path = cv_str(val)) != NULL) {
                    if(glob(path, GLOB_TILDE, NULL, &files) == 0) {
                        for(i = 0; i < files.gl_pathc; i++)
                            scm_c_primitive_load(files.gl_pathv[i]);
                    }
                    globfree(&files);
                }
            }
        }
    }

    return PLUGIN_OK;
}

extern void
unload()
{
    // TODO: cleanup here.
}

/*
 * Run the registered guile trap closure with the grey tuple. This
 * is esentially the C wrapper around guile traps. This is wrapped
 * in a try/catch to catch any runtime exceptions and emit a warning.
 */
static int
guile_spamtrap(struct Grey_tuple *gt, void *arg)
{
    struct trap_data tdata;
    tdata.gt = gt;
    tdata.trap = (SCM) arg;
    SCM result = scm_internal_catch(
        scm_from_bool(1), spamtrap_try, &tdata, spamtrap_catch, NULL);

    return scm_to_int(result);
}

static SCM
spamtrap_try(void *data)
{
    struct trap_data *tdata = data;
    struct Grey_tuple *gt = tdata->gt;
    SCM trap = tdata->trap;
    SCM result = scm_apply_3(
        trap,
        scm_from_locale_string(gt->ip),
        scm_from_locale_string(gt->helo),
        scm_from_locale_string(gt->from),
        scm_cons(scm_from_locale_string(gt->to), SCM_EOL));

    return result;
}

static SCM
spamtrap_catch(void *data, SCM key, SCM args)
{
    char *ckey = scm_to_locale_string(
        scm_symbol_to_string(key));
    i_warning("caught guile exception; %s", ckey);
    free(ckey);

    return scm_from_int(0);
}

/*
 * Export functions to guile so we can register traps from scheme.
 */
static void
*register_api(void *args)
{
    scm_c_define_gsubr(
        "register-spamtrap", 2, 0, 0, &api_register_spamtrap);
    scm_c_define_gsubr(
        "debug", 1, 0, 0, &api_debug);
    scm_c_define_gsubr(
        "warn", 1, 0, 0, &api_warn);
    scm_c_define_gsubr(
        "info", 1, 0, 0, &api_info);

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
api_warn(SCM str)
{
    char *msg = scm_to_locale_string(str);
    i_warning(msg);
    free(msg);
    return SCM_UNSPECIFIED;
}

static SCM
api_info(SCM str)
{
    char *msg = scm_to_locale_string(str);
    i_info(msg);
    free(msg);
    return SCM_UNSPECIFIED;
}

static SCM
api_register_spamtrap(SCM name, SCM callback)
{
    char *plugin_name = scm_to_locale_string(name);
    Plugin_register_spamtrap(
        plugin_name, guile_spamtrap, (void *) callback);
    free(plugin_name);

    return SCM_UNSPECIFIED;
}
