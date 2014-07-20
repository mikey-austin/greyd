/**
 * @file   config_lexer.h
 * @brief  Defines the configuration file lexer interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_LEXER_DEFINED
#define CONFIG_LEXER_DEFINED

#include "config_source.h"

#define T Config_lexer_T

#define CONFIG_LEXER_MAX_TOK_LEN   50
#define CONFIG_LEXER_TOK_STR       100
#define CONFIG_LEXER_TOK_NUM       101
#define CONFIG_LEXER_TOK_EQ        102
#define CONFIG_LEXER_TOK_NAME      103
#define CONFIG_LEXER_TOK_SECTION   104
#define CONFIG_LEXER_TOK_INCLUDE   105
#define CONFIG_LEXER_TOK_BRACKET_L 106
#define CONFIG_LEXER_TOK_BRACKET_R 107

/**
 * The main config lexer structure.
 */
typedef struct T *T;
struct T {
    /* Semantic value of current scanned token. */
    union {
        int  i;
        char s[CONFIG_LEXER_MAX_TOK_LEN + 1];
    } current_value;

    int current_token;
    int seen_end;

    Config_source_T source;
};

extern T    Config_lexer_create(Config_source_T source);
extern void Config_lexer_destroy(T lexer);
extern void Config_lexer_next_token(T lexer);

#undef T
#endif
