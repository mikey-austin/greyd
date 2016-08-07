/*
 * Copyright (c) 2014, 2015 Mikey Austin <mikey@greyd.org>
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
 * @file   mod.h
 * @brief  Defines the interface for dynamically loaded module management.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef MOD_DEFINED
#define MOD_DEFINED

#include "failures.h"
#include "config_section.h"

/**
 * Load the driver module from the specified configuration section, and
 * return an open handle.
 */
extern void *Mod_open(Config_section_T section);

/**
 * Close a previously opened module handle.
 */
extern void Mod_close(void *handle);

/**
 * Fetch a symbol from the module.
 */
extern void *Mod_get(void *handle, const char *sym);

/**
 * Check for module errors and return if defined.
 */
extern const char *Mod_error(void);

#endif
