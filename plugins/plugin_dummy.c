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

#include "failures.h"
#include "plugin.h"
#include "grey.h"
#include "config_section.h"

extern int load(Config_section_T);
extern void unload();

extern int Test_loaded;
extern int Test_spamtrap;
extern int Test_hook;

static void test_hook(void *);
static int test_spamtrap(struct Grey_tuple *, void *);

extern int
load(Config_section_T section)
{
    i_info("in dummy load");

    /* Register hook & spamtrap. */
    Plugin_register_callback(
        HOOK_NEW_ENTRY, "dummy_hook", test_hook, &Test_hook);
    Plugin_register_spamtrap(
        "dummy_spamtrap", test_spamtrap, &Test_spamtrap);
    Test_loaded = 1;

    return PLUGIN_OK;
}

extern void
unload()
{
    Test_loaded = 0;
    i_info("in dummy unload");
}

static void
test_hook(void *arg)
{
    int *hook = (int *) arg;
    (*hook)++;
}

static int
test_spamtrap(struct Grey_tuple *gt, void *arg)
{
    int *spamtrap = (int *) arg;
    (*spamtrap)++;
    return 1;
}
