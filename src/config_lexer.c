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
 * @file   config_lexer.c
 * @brief  Implements the configuration file lexer interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "config_lexer.h"
#include "failures.h"
#include "lexer_source.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Lexer_T
Config_lexer_create(Lexer_source_T source)
{
    Lexer_T lexer;

    lexer = Lexer_create(source, Config_lexer_next_token);

    return lexer;
}

extern int
Config_lexer_next_token(Lexer_T lexer)
{
    int c, seen_esc = 0, len = 0, i;

    /*
     * Scan for the next token.
     */
    for (;;) {
        if (lexer->seen_end) {
            return CONFIG_LEXER_TOK_EOF;
        }

        c = L_GETC(lexer);

        if (isdigit(c)) {
            /*
             * This is an integer.
             */

            for (i = (c - '0'); isdigit(c = L_GETC(lexer));) {
                i = (i * 10) + (c - '0');
            }

            lexer->current_value.i = i;
            Lexer_reuse_char(lexer, c);

            return (lexer->current_token = CONFIG_LEXER_TOK_INT);
        }

        if (isalpha(c)) {
            /*
             * This is either a value name or a reserved word.
             */

            do {
                lexer->current_value.s[len++] = tolower(c);
            } while ((isalpha(c = L_GETC(lexer)) || isdigit(c) || c == '_')
                && len <= LEXER_MAX_STR_LEN);

            lexer->current_value.s[len] = '\0';
            Lexer_reuse_char(lexer, c);

            /* Search for reserved words. */
            if (L_MATCH(lexer->current_value.s, len, "section")) {
                return (lexer->current_token = CONFIG_LEXER_TOK_SECTION);
            } else if (L_MATCH(lexer->current_value.s, len, "include")) {
                return (lexer->current_token = CONFIG_LEXER_TOK_INCLUDE);
            } else if (L_MATCH(lexer->current_value.s, len, "blacklist")) {
                return (lexer->current_token = CONFIG_LEXER_TOK_BLACKLIST);
            } else if (L_MATCH(lexer->current_value.s, len, "whitelist")) {
                return (lexer->current_token = CONFIG_LEXER_TOK_WHITELIST);
            } else {
                /*
                 * The scanned token is not a reserved word and is not a
                 * string value, so it must be a configuration variable name.
                 */

                return (lexer->current_token = CONFIG_LEXER_TOK_NAME);
            }
        }

        /*
         * Process remaining characters.
         */
        switch (c) {
        case ' ':
        case '\t':
        case '\r':
            /*
             * Ignore whitespace.
             */
            break;

        case '#':
            /*
             * Ignore everything until the end of the line (or end of file).
             */

            while ((c = L_GETC(lexer)) != '\n' && c != EOF)
                ;
            Lexer_reuse_char(lexer, c);

            continue;

        case '"':
            /*
             * As we are in a string, populate current string value, while accounting for escaped
             * quote characters.
             */

            while (((c = L_GETC(lexer)) != '"' || seen_esc) && len <= LEXER_MAX_STR_LEN && c != EOF) {
                if (c == '\\' && !seen_esc) {
                    seen_esc = 1;
                } else {
                    /* Add the character to the current value if there is space. */
                    lexer->current_value.s[len++] = c;

                    /* Re-set the escape flag. */
                    seen_esc = 0;
                }
            }

            if (c != '"') {
                /* We don't need the closing quote character. */
                Lexer_reuse_char(lexer, c);
            }

            /* Close off the string. */
            lexer->current_value.s[len] = '\0';

            return (lexer->current_token = CONFIG_LEXER_TOK_STR);

        case ';':
        case '\n':
            return (lexer->current_token = CONFIG_LEXER_TOK_EOL);

        case ',':
            return (lexer->current_token = CONFIG_LEXER_TOK_COMMA);

        case '=':
            return (lexer->current_token = CONFIG_LEXER_TOK_EQ);

        case '{':
            return (lexer->current_token = CONFIG_LEXER_TOK_BRACKET_L);

        case '}':
            return (lexer->current_token = CONFIG_LEXER_TOK_BRACKET_R);

        case '[':
            return (lexer->current_token = CONFIG_LEXER_TOK_SQBRACK_L);

        case ']':
            return (lexer->current_token = CONFIG_LEXER_TOK_SQBRACK_R);

        default:
            /* Unknown character. */
            return CONFIG_LEXER_TOK_EOF;
        }
    }
}
