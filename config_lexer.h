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

#define CONFIG_LEXER_MAX_STR_LEN   256

#define CONFIG_LEXER_TOK_STR       100
#define CONFIG_LEXER_TOK_INT       101
#define CONFIG_LEXER_TOK_EQ        102
#define CONFIG_LEXER_TOK_NAME      103
#define CONFIG_LEXER_TOK_SECTION   104
#define CONFIG_LEXER_TOK_INCLUDE   105
#define CONFIG_LEXER_TOK_BRACKET_L 106
#define CONFIG_LEXER_TOK_BRACKET_R 107
#define CONFIG_LEXER_TOK_EOF       108
#define CONFIG_LEXER_TOK_EOL       109
#define CONFIG_LEXER_TOK_COMMA     110
#define CONFIG_LEXER_TOK_SQBRACK_L 111
#define CONFIG_LEXER_TOK_SQBRACK_R 112

union Config_lexer_token_value {
    int  i;
    char s[CONFIG_LEXER_MAX_STR_LEN + 1];
};

/**
 * The main config lexer structure.
 */
typedef struct T *T;
struct T {
    /* Semantic value of current scanned token. */
    union Config_lexer_token_value current_value;
    union Config_lexer_token_value previous_value;

    int current_token;
    int previous_token;
    int seen_end;

    Config_source_T source;
};

/**
 * Create a fresh lexer object.
 */
extern T Config_lexer_create(Config_source_T source);

/**
 * Destroy a lexer object. This will automatically destroy the associated
 * configuration source object.
 */
extern void Config_lexer_destroy(T lexer);

/**
 * Scan the specified configuration source's character stream and return
 * the next token seen.
 */
extern int Config_lexer_next_token(T lexer);

#undef T
#endif
