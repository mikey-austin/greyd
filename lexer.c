/**
 * @file   config_lexer.c
 * @brief  Implements the configuration file lexer interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "lexer_source.h"
#include "config_lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define T Lexer_T

extern T
Lexer_create(Lexer_source_T source, int (*next_token)(T lexer))
{
    T lexer;

    if((lexer = malloc(sizeof(*lexer))) == NULL) {
        I_CRIT("Could not create lexer");
    }

    lexer->source           = source;
    lexer->seen_end         = 0;
    lexer->current_line     = 0;
    lexer->current_line_pos = 0;
    lexer->prev_line_pos    = 0;

    lexer->next_token = next_token;

    return lexer;
}

extern int
Lexer_next_token(T lexer)
{
    return lexer->next_token(lexer);
}

extern void
Lexer_destroy(T *lexer)
{
    if(lexer == NULL || *lexer == NULL)
        return;

    if((*lexer)->source)
        Lexer_source_destroy(&((*lexer)->source));

    free(*lexer);
    *lexer = NULL;
}

extern void
Lexer_reuse_char(T lexer, int c)
{
    if(c == EOF || c < 0) {
        /* We can't reuse an EOF character. */
        lexer->seen_end = 1;
    }
    else {
        L_UNGETC(lexer, c);
    }
}

extern int
Lexer_getc(T lexer)
{
    int c;

    c = Lexer_source_getc(lexer->source);
    lexer->prev_line_pos = lexer->current_line_pos;
    switch(c)
    {
    case '\n':
        lexer->current_line++;
        lexer->current_line_pos = 0;
        break;

    default:
        lexer->current_line_pos++;
    }

    return c;
}

extern void
Lexer_ungetc(T lexer, int c)
{
    if(c == '\n') {
        lexer->current_line--;
        lexer->current_line_pos = lexer->prev_line_pos;
    }
    else if(lexer->current_line_pos > 0) {
        lexer->current_line_pos--;
    }

    Lexer_source_ungetc(lexer->source, c);
}

#undef T
