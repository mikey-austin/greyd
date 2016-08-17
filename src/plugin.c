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

#include "mod.h"
#include "plugin.h"
#include "list.h"
#include "hash.h"
#include "failures.h"

#define PLUGIN_INIT_NUM 5

static List_T Plugin_hooks[PLUGIN_NUM_HOOKS];
static List_T Plugin_spamtraps;
static Hash_T Plugin_loaded;
static int    Plugin_enabled;

static void destroy_plugin(struct Hash_entry *);
static void destroy_callback(void *);
static void destroy_trap(void *);

struct Plugin {
    void *driver;
    void (*unload)(void);
};

struct Plugin_callback {
    void (*cb)(void *);
    void *arg;
    char *name;
};

struct Plugin_trap {
    int (*trap)(struct Grey_tuple *, void *);
    void *arg;
    char *name;
};

extern void
Plugin_sys_init(Config_T config)
{
    List_T plugins = NULL;
    Config_section_T plugin_section = NULL;
    struct List_entry *entry;
    struct Plugin *plugin = NULL;
    void *driver = NULL;
    int (*load)(Config_section_T) = NULL;
    void (*unload)(void) = NULL;
    int i;

    /* Setup the plugin environment. */
    if(!(Plugin_enabled = Config_get_int(config, "enable_plugins", NULL, 0)))
        return;

    Plugin_loaded = Hash_create(PLUGIN_INIT_NUM, destroy_plugin);
    Plugin_spamtraps = NULL;
    for(i = 0; i < PLUGIN_NUM_HOOKS; i++)
        Plugin_hooks[i] = NULL;

    /* Load the actual plugins. */
    if(Plugin_enabled
       && (plugins = Config_get_all_plugins(config)))
    {
        LIST_EACH(plugins, entry) {
            plugin_section = List_entry_value(entry);
            if((driver = Mod_open(plugin_section)) != NULL) {
                load = (int (*)(Config_section_T)) Mod_get(driver, "load");
                unload = (void (*)(void)) Mod_get(driver, "unload");
                if(load && load(plugin_section) == PLUGIN_OK) {
                    /* Store the plugin so we can unload it. */
                    if((plugin = malloc(sizeof(*plugin))) == NULL)
                        i_critical("Could not store plugin: %s", strerror(errno));
                    plugin->driver = driver;
                    plugin->unload = unload; /* The unload is optional. */
                    Hash_insert(Plugin_loaded, plugin_section->name, plugin);

                    i_info("loaded %s plugin", plugin_section->name);
                }
                else {
                    i_warning("could not initialize %s plugin",
                              plugin_section->name);
                }
            }
            else {
                i_warning("could not load %s plugin: %s",
                          plugin_section->name,
                          Mod_error());
            }
        }

        List_destroy(&plugins);
    }
}

extern void
Plugin_sys_stop(void)
{
    int i;

    if(!Plugin_enabled)
        return;

    /* The hash value destructor will take care of unloading. */
    Hash_destroy(&Plugin_loaded);

    if(Plugin_spamtraps != NULL)
        List_destroy(&Plugin_spamtraps);

    for(i = 0; i < PLUGIN_NUM_HOOKS; i++)
        if(Plugin_hooks[i] != NULL)
            List_destroy(&Plugin_hooks[i]);
}

extern void
Plugin_register_callback(
    enum Plugin_hook hook, const char *name, void (*cb)(void *),
    void *arg)
{
    struct Plugin_callback *pcb = NULL;

    if(!Plugin_enabled)
        return;

    if((pcb = malloc(sizeof(*pcb))) == NULL)
        i_critical("Could not create callback: %s", strerror(errno));

    pcb->name = strdup(name);
    pcb->cb = cb;
    pcb->arg = arg;

    if(Plugin_hooks[hook] == NULL)
        Plugin_hooks[hook] = List_create(destroy_callback);
    List_insert_after(Plugin_hooks[hook], (void *) pcb);

    i_debug("registered callback %s", name);
}

extern void
Plugin_register_spamtrap(
    const char *name, int (*trap)(struct Grey_tuple *, void *),
    void *arg)
{
    struct Plugin_trap *spamtrap = NULL;

    if(!Plugin_enabled)
        return;

    if((spamtrap = malloc(sizeof(*spamtrap))) == NULL)
        i_critical("Could not create spamtrap: %s", strerror(errno));

    spamtrap->name = strdup(name);
    spamtrap->trap = trap;
    spamtrap->arg = arg;

    if(Plugin_spamtraps == NULL)
        Plugin_spamtraps = List_create(destroy_trap);
    List_insert_after(Plugin_spamtraps, (void *) spamtrap);

    i_debug("registered spamtrap %s", name);
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
        i_debug("running hook %s", callback->name);
        callback->cb(callback->arg);
    }
}

extern int
Plugin_run_spamtraps(struct Grey_tuple *gt)
{
    struct List_entry *entry;
    struct Plugin_trap *trap;

    if(Plugin_spamtraps && List_size(Plugin_spamtraps) > 0) {
        LIST_EACH(Plugin_spamtraps, entry) {
            trap = List_entry_value(entry);
            if(trap->trap(gt, trap->arg)) {
                i_debug("spamtrap %s triggered", trap->name);
                return 0;
            }
        }
    }

    return 1;
}

static void
destroy_callback(void *callback)
{
    struct Plugin_callback *p = (struct Plugin_callback *) callback;

    if(p != NULL) {
        if(p->name)
            free(p->name);
        free(p);
    }
}

static void
destroy_trap(void *trap)
{
    struct Plugin_trap *p = (struct Plugin_trap *) trap;

    if(p != NULL) {
        if(p->name)
            free(p->name);
        free(p);
    }
}

static void
destroy_plugin(struct Hash_entry *entry)
{
    struct Plugin *plugin = NULL;

    if(entry && (plugin = (struct Plugin *) entry->v)) {
        /* Call unload callback if one exists. */
        if(plugin->unload)
            plugin->unload();
        if(plugin->driver)
            Mod_close(plugin->driver);
        free(plugin);
        entry->v = NULL;
    }
}
