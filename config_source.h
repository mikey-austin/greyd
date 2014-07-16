/**
 * @file   config_source.h
 * @brief  Defines interface for configuration source data.
 * @author Mikey Austin
 * @date   2014
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

extern T    Config_create_from_file(const char *filename);

extern T    Config_create_from_str(const char *buf);

extern void Config_source_destroy(T source);

extern int  Config_source_getc(T source);

extern void Config_source_ungetc(T source, int c);

#undef T
#endif
