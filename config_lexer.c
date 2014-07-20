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

extern void
Config_lexer_next_token(T lexer)
{
    int c;

    if(lexer->seen_end)
        return;

    for(;;) {
        c = L_GETC(lexer);

        switch(c) {
        case '#':
            /* Ignore everything until the end of the line (or end of file). */
            while((c = L_GETC(lexer)) != '\n' || c != EOF)
                ;
            break;

        case '"':
            /* As we are in a string, populate current string value. */
            lexer->current_token = CONFIG_LEXER_TOK_STR;
            break;
        }
    }
}

#undef T
