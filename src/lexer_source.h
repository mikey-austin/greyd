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
 * @file   lexer_source.h
 * @brief  Defines interface for configuration source data.
 * @author Mikey Austin
 * @date   2014
 *
 * This interface abstracts a character stream from the actual source, providing
 * a common interface for NULL-terminated string buffers and files.
 */

#ifndef LEXER_SOURCE_DEFINED
#define LEXER_SOURCE_DEFINED

#include <zlib.h>

#define LEXER_SOURCE_STR  1
#define LEXER_SOURCE_FILE 2
#define LEXER_SOURCE_GZ   3

typedef struct Lexer_source_T *Lexer_source_T;
struct Lexer_source_T {
    int    type;
    void  *data;
    int  (*_getc)(void *data);
    void (*_ungetc)(void *data, int c);
    void (*_destroy)(void *data);
    int (*_error)(void *data);
    void (*_clear_error)(void *data);
};

/**
 * Create a new configuration source from an absolute path.
 */
extern Lexer_source_T Lexer_source_create_from_file(const char *filename);

/**
 * Create a new configuration source from an open file descriptor.
 */
extern Lexer_source_T Lexer_source_create_from_fd(int fd);

/**
 * Create a new configuration source from a NULL-terminated buffer. Note,
 * the buffer will be copied into the newly created source object.
 */
extern Lexer_source_T Lexer_source_create_from_str(const char *buf, int len);

/**
 * Create a new configuration source from a gzipped file.
 */
extern Lexer_source_T Lexer_source_create_from_gz(gzFile gzf);

/**
 * Destroy a source object and it's data.
 */
extern void Lexer_source_destroy(Lexer_source_T *source);

/**
 * Get the next character from this source's queue. An EOF will be returned
 * upon the end of the stream.
 */
extern int Lexer_source_getc(Lexer_source_T source);

/**
 * Push a character back onto the front of the source's stream. Note, calling
 * this function multiple times before a getc will yield undefined results.
 */
extern void Lexer_source_ungetc(Lexer_source_T source, int c);

/**
 * Return 1 if an error occurred, 0 otherwise.
 */
extern int Lexer_source_error(Lexer_source_T source);

/**
 * Clear any error in stream.
 */
extern void Lexer_source_clear_error(Lexer_source_T source);

#endif
