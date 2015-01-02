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

#include "failures.h"
#include "lexer_source.h"
#include "config_lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern Lexer_T
Lexer_create(Lexer_source_T source, int (*next_token)(Lexer_T lexer))
{
    Lexer_T lexer;

    if((lexer = malloc(sizeof(*lexer))) == NULL) {
        i_critical("Could not create lexer");
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
Lexer_next_token(Lexer_T lexer)
{
    return lexer->next_token(lexer);
}

extern void
Lexer_destroy(Lexer_T *lexer)
{
    if(lexer == NULL || *lexer == NULL)
        return;

    if((*lexer)->source)
        Lexer_source_destroy(&((*lexer)->source));

    free(*lexer);
    *lexer = NULL;
}

extern void
Lexer_reuse_char(Lexer_T lexer, int c)
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
Lexer_getc(Lexer_T lexer)
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
Lexer_ungetc(Lexer_T lexer, int c)
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
