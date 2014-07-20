/**
 * @file   config_source.h
 * @brief  Defines interface for configuration source data.
 * @author Mikey Austin
 * @date   2014
 *
 * This interface abstracts a character stream from the actual source, providing
 * a common interface for NULL-terminated string buffers and files.
 */

#ifndef CONFIG_SOURCE_DEFINED
#define CONFIG_SOURCE_DEFINED

#define T Config_source_T

#define CONFIG_SOURCE_STR  1
#define CONFIG_SOURCE_FILE 2

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
extern T Config_source_create_from_file(const char *filename);

/**
 * Create a new configuration source from a NULL-terminated buffer. Note,
 * the buffer will be copied into the newly created source object.
 */
extern T Config_source_create_from_str(const char *buf);

/**
 * Destroy a source object and it's data.
 */
extern void Config_source_destroy(T source);

/**
 * Get the next character from this source's queue. An EOF will be returned
 * upon the end of the stream.
 */
extern int Config_source_getc(T source);

/**
 * Push a character back onto the front of the source's stream. Note, calling
 * this function multiple times before a getc will yield undefined results.
 */
extern void Config_source_ungetc(T source, int c);

#undef T
#endif
