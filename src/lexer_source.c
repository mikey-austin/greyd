/**
 * @file   lexer_source.c
 * @brief  Implements interface for configuration source data.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "failures.h"
#include "lexer_source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct source_data_file {
    char *filename;
    FILE *handle;
};

struct source_data_str {
    const char *buf;
    int         index;
    int         length;
};

struct source_data_gz {
    gzFile gzf;
};

static Lexer_source_T source_create();
static void  source_data_file_destroy(void *data);
static int   source_data_file_getc(void *data);
static void  source_data_file_ungetc(void *data, int c);

static void  source_data_str_destroy(void *data);
static int   source_data_str_getc(void *data);
static void  source_data_str_ungetc(void *data, int c);

static void  source_data_gz_destroy(void *data);
static int   source_data_gz_getc(void *data);
static void  source_data_gz_ungetc(void *data, int c);

extern Lexer_source_T
Lexer_source_create_from_file(const char *filename)
{
    int flen = strlen(filename) + 1;
    Lexer_source_T source = source_create();
    struct source_data_file *data;

    data = malloc(sizeof(*data));
    if(data == NULL) {
        i_critical("Could not create config source for %s", filename);
    }
    else {
        /* Store the filename. */
        data->filename = malloc(flen);
        if(data->filename == NULL) {
            i_critical("Could not create file config source");
        }
        sstrncpy(data->filename, filename, flen);

        /* Try to open the file. */
        data->handle = fopen(filename, "r");
        if(data->handle == NULL) {
            i_critical("Error opening config file source: %s", strerror(errno));
        }

        source->type     = LEXER_SOURCE_FILE;
        source->data     = data;
        source->_getc    = source_data_file_getc;
        source->_ungetc  = source_data_file_ungetc;
        source->_destroy = source_data_file_destroy;
    }

    return source;
}

extern Lexer_source_T
Lexer_source_create_from_fd(int fd)
{
    Lexer_source_T source = source_create();
    struct source_data_file *data;

    data = malloc(sizeof(*data));
    if(data == NULL) {
        i_critical("Could not create config source for fd %d", fd);
    }
    else {
        /* Try to open the file. */
        data->handle = fdopen(fd, "r");
        if(data->handle == NULL) {
            i_critical("Error opening config file source: %s", strerror(errno));
        }

        /* No need for a filename. */
        data->filename = NULL;

        source->type     = LEXER_SOURCE_FILE;
        source->data     = data;
        source->_getc    = source_data_file_getc;
        source->_ungetc  = source_data_file_ungetc;
        source->_destroy = source_data_file_destroy;
    }

    return source;
}

extern Lexer_source_T
Lexer_source_create_from_str(const char *buf, int len)
{
    Lexer_source_T source = source_create();
    struct source_data_str *data;

    data = malloc(sizeof(*data));
    if(data == NULL) {
        i_critical("Could not create config source");
    }
    else {
        /* Copy the buffer into the new source object. */
        data->buf = buf;

        data->index  = 0;    /* Index is for traversing buffer. */
        data->length = len;

        source->type     = LEXER_SOURCE_STR;
        source->data     = data;
        source->_getc    = source_data_str_getc;
        source->_ungetc  = source_data_str_ungetc;
        source->_destroy = source_data_str_destroy;
    }

    return source;
}

extern Lexer_source_T
Lexer_source_create_from_gz(gzFile gzf)
{
    Lexer_source_T source = source_create();
    struct source_data_gz *data;

    data = malloc(sizeof(*data));
    if(data == NULL) {
        i_critical("Could not create config source");
    }
    else {
        /* Store the reference to the gz file handle. */
        data->gzf = gzf;

        source->type     = LEXER_SOURCE_GZ;
        source->data     = data;
        source->_getc    = source_data_gz_getc;
        source->_ungetc  = source_data_gz_ungetc;
        source->_destroy = source_data_gz_destroy;
    }

    return source;
}

extern void
Lexer_source_destroy(Lexer_source_T *source)
{
    if(source == NULL || *source == NULL)
        return;

    (*source)->_destroy((*source)->data);
    free(*source);
    *source = NULL;
}

extern int
Lexer_source_getc(Lexer_source_T source)
{
    return source->_getc(source->data);
}

extern void
Lexer_source_ungetc(Lexer_source_T source, int c)
{
    source->_ungetc(source->data, c);
}

static Lexer_source_T
source_create()
{
    Lexer_source_T source;

    source = malloc(sizeof(*source));
    if(source == NULL) {
        i_critical("Could not create config source");
    }
    else {
        /* Initialize object. */
        source->type = -1;
        source->data = NULL;
    }

    return source;
}

static void
source_data_file_destroy(void *data)
{
    struct source_data_file *data_file;
    if(data == NULL)
        return;

    data_file = (struct source_data_file *) data;
    if(data_file->filename != NULL) {
        free(data_file->filename);
        data_file->filename = NULL;
    }

    /* Try to close the handle. */
    if(fclose(data_file->handle) == EOF) {
        i_warning("Error closing config source: %s", strerror(errno));
    }

    free(data_file);
    data_file = NULL;
}

static int
source_data_file_getc(void *data)
{
    struct source_data_file *data_file = (struct source_data_file *) data;
    return getc(data_file->handle);
}

static void
source_data_file_ungetc(void *data, int c)
{
    struct source_data_file *data_file = (struct source_data_file *) data;
    ungetc(c, data_file->handle);
}

static void
source_data_str_destroy(void *data)
{
    struct source_data_str *data_str;
    if(data == NULL)
        return;

    data_str = (struct source_data_str *) data;
    free(data_str);
    data_str = NULL;
}

/*
 * The following string source getc and ungetc functions simulate a queue
 * by moving the index back and forth.
 */
static int
source_data_str_getc(void *data)
{
    struct source_data_str *data_str = (struct source_data_str *) data;

    if(data_str->index <= (data_str->length - 1)) {
        return data_str->buf[data_str->index++];
    }

    return EOF;
}

static void
source_data_str_ungetc(void *data, int c)
{
    struct source_data_str *data_str = (struct source_data_str *) data;
    int prev = data_str->index - 1;

    if(prev >= 0) {
        data_str->index = prev;
    }
}

static void
source_data_gz_destroy(void *data)
{
    int ret;
    struct source_data_gz *data_gz = (struct source_data_gz *) data;

    if(data_gz == NULL)
        return;

    if(data_gz->gzf && ((ret = gzclose(data_gz->gzf)) != Z_OK)) {
        i_warning("gzclose returned an unexpected %d", ret);
    }

    free(data_gz);
    data_gz = NULL;
}

static int
source_data_gz_getc(void *data)
{
   struct source_data_gz *data_gz = (struct source_data_gz *) data;
   return gzgetc(data_gz->gzf);
}

static void
source_data_gz_ungetc(void *data, int c)
{
   struct source_data_gz *data_gz = (struct source_data_gz *) data;
   gzungetc(c, data_gz->gzf);
}
