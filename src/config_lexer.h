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
 * @file   config_lexer.h
 * @brief  Defines the configuration file lexer interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_LEXER_DEFINED
#define CONFIG_LEXER_DEFINED

#include "lexer.h"

#define CONFIG_LEXER_MAX_STR_LEN   256

#define CONFIG_LEXER_TOK_STR       100
#define CONFIG_LEXER_TOK_INT       101
#define CONFIG_LEXER_TOK_EQ        102
#define CONFIG_LEXER_TOK_NAME      103
#define CONFIG_LEXER_TOK_SECTION   104
#define CONFIG_LEXER_TOK_INCLUDE   105
#define CONFIG_LEXER_TOK_BRACKET_L 106
#define CONFIG_LEXER_TOK_BRACKET_R 107
#define CONFIG_LEXER_TOK_EOF       108
#define CONFIG_LEXER_TOK_EOL       109
#define CONFIG_LEXER_TOK_COMMA     110
#define CONFIG_LEXER_TOK_SQBRACK_L 111
#define CONFIG_LEXER_TOK_SQBRACK_R 112
#define CONFIG_LEXER_TOK_BLACKLIST 113
#define CONFIG_LEXER_TOK_WHITELIST 114
#define CONFIG_LEXER_TOK_PLUGIN    115

/**
 * Create a fresh lexer object.
 */
extern Lexer_T Config_lexer_create(Lexer_source_T source);

/**
 * Scan the specified configuration source's character stream and return
 * the next token seen.
 */
extern int Config_lexer_next_token(Lexer_T lexer);

#endif
