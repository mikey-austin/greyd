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
 * @file   config_parser.h
 * @brief  Defines the configuration file parser interface.
 * @author Mikey Austin
 * @date   2014
 *
 * The config grammar is as follows (in BNF form, terminals uppercase):
 *
 * @code
 * config : statement statements EOF
 *        | EOF
 *        ;
 *
 * statements : EOL statement statements
 *            |
 *            ;
 *
 * statement : assignment
 *           | section
 *           | include
 *           ;
 *
 * assignment : NAME = INT
 *            | NAME = STR
 *            | NAME = list
 *            ;
 *
 * list : [ list_statements ]
 *      | [ list_statements EOL ]
 *      | [ EOL list_statements ]
 *      | [ EOL list_statements EOL ]
 *      ;
 *
 * list_statements : list_value list_values
 *                 ;
 *
 * list_values : , EOL list_value list_values
 *             | , list_value list_values
 *             |
 *             ;
 *
 * list_value : INT
 *            | STR
 *            ;
 *
 * section : section_type NAME { EOL section_statements EOL }
 *         | section_type NAME EOL { EOL section_statements EOL }
 *         ;
 *
 * section_type : SECTION
 *              | BLACKLIST
 *              | WHITELIST
 *              ;
 *
 * section_statements : assignment section_assignments
 *                    ;
 *
 * section_assignments : , EOL assignment section_assignments
 *                     |
 *                     ;
 *
 * include : INCLUDE STR
 *         ;
 * @endcode
 */

#ifndef CONFIG_PARSER_DEFINED
#define CONFIG_PARSER_DEFINED

#include "config_lexer.h"
#include "greyd_config.h"

#define T Config_parser_T

#define CONFIG_PARSER_OK  1
#define CONFIG_PARSER_ERR 0

/**
 * The main config parser structure.
 */
typedef struct T *T;
struct T {
    Config_T         config;  /**< Reference to the config object. */
    Config_section_T section; /**< Reference to the current section. */
    Config_value_T   value;   /**< Reference to the current config value. */
    Lexer_T          lexer;
    int              curr;    /**< The current token being looked at. */
    int              sectype; /**< The type of section being parsed. */
};

/**
 * Create a fresh parser object.
 *
 * @param lexer The initialized token stream.
 *
 * @return An initialized parser object.
 */
extern T Config_parser_create(Lexer_T lexer);

/**
 * Destroy a parser object. This will automatically destroy the associated
 * configuration source object.
 *
 * @param parser The initialized parser object to be destroyed.
 */
extern void Config_parser_destroy(T *parser);

/**
 * Start parsing the lexer's token stream, and populate the specified config object.
 *
 * @param parser The initialized parser object.
 * @param config The initialized configuration object to populate.
 *
 * @return 1 if there are no problems, 0 otherwise.
 */
extern int Config_parser_start(T parser, Config_T config);

#undef T
#endif
