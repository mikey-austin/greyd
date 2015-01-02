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
 * @file   lexer.h
 * @brief  Defines the generic lexer interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef LEXER_DEFINED
#define LEXER_DEFINED

#include "lexer_source.h"

#define LEXER_MAX_STR_LEN   256

/*
 * Convenience macros.
 */
#define L_GETC(lexer)      Lexer_getc(lexer)
#define L_UNGETC(lexer, c) Lexer_ungetc(lexer, c)
#define L_MATCH(a, len, b) (len == strlen(b) && strncmp(a, b, len) == 0)

union Lexer_token_value {
    int  i;
    char s[LEXER_MAX_STR_LEN + 1];
};

/**
 * The main config lexer structure.
 */
typedef struct Lexer_T *Lexer_T;
struct Lexer_T {
    /* Semantic value of current scanned token. */
    union Lexer_token_value current_value;

    int current_token;
    int seen_end;

    /* Line information. */
    int current_line;
    int current_line_pos;
    int prev_line_pos;

    int (*next_token)(Lexer_T lexer);

    Lexer_source_T source;
};

/**
 * Create a fresh lexer object.
 */
extern Lexer_T Lexer_create(Lexer_source_T source, int (*next_token)(Lexer_T lexer));

/**
 * Destroy a lexer object. This will automatically destroy the associated
 * configuration source object.
 */
extern void Lexer_destroy(Lexer_T *lexer);

/**
 * Scan the specified configuration source's character stream and return
 * the next token seen.
 */
extern int Lexer_next_token(Lexer_T lexer);

/**
 * Push the character back onto the front of the stream, to be used again,
 * with the exception of end of stream characters.
 */
extern void Lexer_reuse_char(Lexer_T lexer, int c);

/**
 * Get the next character from the stream and update the various counters.
 */
extern int Lexer_getc(Lexer_T lexer);

/**
 * Unget the character from the stream and maintain the counters accordingly.
 */
extern void Lexer_ungetc(Lexer_T lexer, int c);

#endif
