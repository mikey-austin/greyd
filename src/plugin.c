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

/**
 * @file   plugin.c
 * @brief  Implements the plugin interface.
 * @author Mikey Austin
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "plugin.h"
#include "list.h"
#include "failures.h"

static List_T Plugin_hooks[PLUGIN_NUM_HOOKS];
static List_T Plugin_spamtraps;

static void destroy_callback(void *);
static void destroy_trap(void *);

struct Plugin_callback {
    void (*cb)(void *);
    void *arg;
};

struct Plugin_trap {
    int (*trap)(struct Grey_tuple *, void *);
    void *arg;
};

extern void
Plugin_sys_init(Config_T config)
{
    int i;

    if(!Config_get_int(config, "enable", "plugins", 0))
        return;

    Plugin_spamtraps = NULL;
    for(i = 0; i < PLUGIN_NUM_HOOKS; i++)
        Plugin_hooks[i] = NULL;
}

extern void
Plugin_register_callback(
    enum Plugin_hook hook, void (*cb)(void *), void *arg)
{
    struct Plugin_callback *pcb = NULL;
    if((pcb = malloc(sizeof(*pcb))) == NULL)
        i_critical("Could not create callback: %s", strerror(errno));

    pcb->cb = cb;
    pcb->arg = arg;

    if(Plugin_hooks[hook] == NULL)
        Plugin_hooks[hook] = List_create(destroy_callback);
    List_insert_after(Plugin_hooks[hook], (void *) pcb);
}

extern int
Plugin_register_spamtrap(
    int (*trap)(struct Grey_tuple *, void *), void *arg)
{
    struct Plugin_trap *spamtrap = NULL;
    if((spamtrap = malloc(sizeof(*spamtrap))) == NULL)
        i_critical("Could not create spamtrap: %s", strerror(errno));

    spamtrap->trap = trap;
    spamtrap->arg = arg;

    if(Plugin_spamtraps == NULL)
        Plugin_spamtraps = List_create(destroy_trap);
    List_insert_after(Plugin_spamtraps, (void *) spamtrap);
}

extern void
Plugin_run_callbacks(enum Plugin_hook hook)
{
    struct List_entry *entry;
    struct Plugin_callback *callback;

    if(!Plugin_hooks[hook] || !List_size(Plugin_hooks[hook]))
        return;

    /* Run each callback in turn. */
    LIST_EACH(Plugin_hooks[hook], entry) {
        callback = List_entry_value(entry);
        callback->cb(callback->arg);
    }
}

extern int
Plugin_run_spamtraps(struct Grey_tuple *gt)
{
    struct List_entry *entry;
    struct Plugin_trap *trap;

    if(!Plugin_spamtraps || !List_size(Plugin_spamtraps))
        return 0;

    LIST_EACH(Plugin_spamtraps, entry) {
        trap = List_entry_value(entry);
        if(trap->trap(gt, trap->arg))
            return 1;
    }

    return 0;
}

static void
destroy_callback(void *callback)
{
    struct Plugin_callback *p = (struct Plugin_callback *) callback;

    if(p != NULL)
        free(p);
}

static void
destroy_trap(void *trap)
{
    struct Plugin_trap *p = (struct Plugin_trap *) trap;

    if(p != NULL)
        free(p);
}
