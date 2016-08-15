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
 * @file   plugin.h
 * @brief  Defines the plugin interface.
 * @author Mikey Austin
 */

#ifndef PLUGIN_DEFINED
#define PLUGIN_DEFINED

#include "greyd_config.h"
#include "grey.h"

#define PLUGIN_NUM_HOOKS 1

#define PLUGIN_OK  0
#define PLUGIN_ERR 1

/**
 * Pre-defined callback hook locations.
 */
enum Plugin_hook {
    HOOK_NEW_ENTRY
};

/**
 * Initialize the plugin system and load all configured
 * plugins.
 */
extern void Plugin_sys_init(Config_T config);

/**
 * Cleanup plugin system.
 */
extern void Plugin_sys_stop(void);

/**
 * Register a callback function to be called at the specified
 * hook location.
 */
extern void Plugin_register_callback(
    enum Plugin_hook hook, const char *, void (*)(void *), void *arg);

/**
 * Register a spamtrap function to be called.
 */
extern void Plugin_register_spamtrap(
    const char *, int (*)(struct Grey_tuple *, void *), void *arg);

/**
 * Run all callbacks registered with the supplied hook to run.
 */
extern void Plugin_run_callbacks(enum Plugin_hook hook);

/**
 * Run all registered spamtraps until either all the traps
 * have been run, or the first trap goes off.
 *
 * @return 0  Address is a spamtrap.
 * @return 1  Address doesn't match a known spamtrap.
 * @return -1 An error occured.
 */
extern int Plugin_run_spamtraps(struct Grey_tuple *);

#endif
