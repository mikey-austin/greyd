/**
 * @file   config_lexer.c
 * @brief  Implements the configuration file lexer interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "config_source.h"
#include "config_lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define T Config_lexer_T

#define L_GETC(lexer)    Config_source_getc(lexer->source)
#define L_UNGETC(lexer, c) Config_source_ungetc(lexer->source, c)

extern T
Config_lexer_create(Config_source_T source)
{
    T lexer;

    if((lexer = (T) malloc(sizeof(*lexer))) == NULL) {
        I_CRIT("Could not create lexer");
    }

    lexer->source   = source;
    lexer->seen_end = 0;

    return lexer;
}

extern void
Config_lexer_destroy(T lexer)
{
    if(!lexer)
        return;

    if(lexer->source)
        Config_source_destroy(lexer->source);

    free(lexer);
}

extern int
Config_lexer_next_token(T lexer)
{
    int c, seen_esc = 0, len = 0, i;

    if(lexer->seen_end)
        return 0;

    for(;;) {
        c = L_GETC(lexer);

        if(c == '#') {
            /*
             * Ignore everything until the end of the line (or end of file).
             */

            while((c = L_GETC(lexer)) != '\n')
                ;

            continue;
        } else if(c == '"') {
            /*
             * As we are in a string, populate current string value.
             */

            while((c = L_GETC(lexer)) != '"' || seen_esc) {
                if(c == '\\' && !seen_esc) {
                    seen_esc = 1;
                    continue;
                }

                /* Add the character to the current value if there is space. */
                if(++len <= CONFIG_LEXER_MAX_STR_LEN) {
                    *(lexer->current_value.s + (len - 1)) = c;
                } else {
                    I_CRIT("Maximum config variable string length reached");
                }

                /* Re-set the escape flag. */
                seen_esc = 0;
            }

            /* Close off the string. */
            *(lexer->current_value.s + (len > 0 && len <= CONFIG_LEXER_MAX_STR_LEN
                                        ? len : CONFIG_LEXER_MAX_STR_LEN)) = '\0';

            return (lexer->current_token = CONFIG_LEXER_TOK_STR);
        } else if(isdigit(c)) {
            /*
             * This is an integer.
             */

            for(i = (c - '0'); isdigit(c = L_GETC(lexer)); ) {
                i = (i * 10) + (c - '0');
            }
            lexer->current_value.i = i;

            return (lexer->current_token = CONFIG_LEXER_TOK_INT);
        }

        /*
         * Process remaining characters.
         */
        switch(c) {
        case ' ':
        case '\t':
            /* Ignore whitespace. */
            break;

        case '\n':
            return (lexer->current_token = CONFIG_LEXER_TOK_NEWLINE);

        case '=':
            return (lexer->current_token = CONFIG_LEXER_TOK_EQ);

        case '{':
            return (lexer->current_token = CONFIG_LEXER_TOK_BRACKET_L);

        case '}':
            return (lexer->current_token = CONFIG_LEXER_TOK_BRACKET_R);

        default:
            /* Unknown character. */
            return 0;
        }
    }
}

#undef T
