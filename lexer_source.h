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

#define T Lexer_source_T

#define LEXER_SOURCE_STR  1
#define LEXER_SOURCE_FILE 2
#define LEXER_SOURCE_GZ   3

typedef struct T *T;
struct T {
    int    type;
    void  *data;
    int  (*_getc)(void *data);
    void (*_ungetc)(void *data, int c);
    void (*_destroy)(void *data);
};

/**
 * Create a new configuration source from an absolute path.
 */
extern T Lexer_source_create_from_file(const char *filename);

/**
 * Create a new configuration source from a NULL-terminated buffer. Note,
 * the buffer will be copied into the newly created source object.
 */
extern T Lexer_source_create_from_str(const char *buf, int len);

/**
 * Create a new configuration source from a gzipped file.
 */
extern T Lexer_source_create_from_gz(gzFile gzf);

/**
 * Destroy a source object and it's data.
 */
extern void Lexer_source_destroy(T *source);

/**
 * Get the next character from this source's queue. An EOF will be returned
 * upon the end of the stream.
 */
extern int Lexer_source_getc(T source);

/**
 * Push a character back onto the front of the source's stream. Note, calling
 * this function multiple times before a getc will yield undefined results.
 */
extern void Lexer_source_ungetc(T source, int c);

#undef T
#endif
