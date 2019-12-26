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
 * @file   spamd_parser.h
 * @brief  Defines the spamd-style backlist/whitelist parser interface.
 * @author Mikey Austin
 * @date   2014
 *
 * @code
 * blacklist : entry entries EOF
 *           | EOF
 *           ;
 *
 * entries : EOL entry entries
 *         |
 *         ;
 *
 * entry : address
 *       | address / INT6
 *       | address - address
 *       ;
 *
 * address : number . number . number . number
 *         ;
 *
 * number : INT6
 *        | INT8
 *        ;
 * @endcode
 */

#ifndef SPAMD_PARSER_DEFINED
#define SPAMD_PARSER_DEFINED

#include "blacklist.h"
#include "ip.h"
#include "spamd_lexer.h"

#include <stdint.h>

#define SPAMD_PARSER_OK 1
#define SPAMD_PARSER_ERR 0

#define SPAMD_PARSER_IPV4_QUADS 4

/**
 * The main spamd parser structure.
 */
typedef struct Spamd_parser_T* Spamd_parser_T;
struct Spamd_parser_T {
    u_int8_t start[SPAMD_PARSER_IPV4_QUADS];
    u_int8_t end[SPAMD_PARSER_IPV4_QUADS];
    Lexer_T lexer;
    Blacklist_T blacklist;
    int type;
    int curr;
};

/**
 * Create a fresh parser object.
 *
 * @param lexer The initialized token stream.
 *
 * @return An initialized parser object.
 */
extern Spamd_parser_T Spamd_parser_create(Lexer_T lexer);

/**
 * Destroy a parser object.
 *
 * @param parser The initialized parser object to be destroyed.
 */
extern void Spamd_parser_destroy(Spamd_parser_T* parser);

/**
 * Start parsing the lexer's token stream, and populate the specified
 * blacklist object.
 *
 * @param parser The initialized parser object.
 * @param config The initialized configuration object to populate.
 *
 * @return 1 if there are no problems, 0 otherwise.
 */
extern int Spamd_parser_start(Spamd_parser_T parser, Blacklist_T blacklist, int type);

#endif
