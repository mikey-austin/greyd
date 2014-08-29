/**
 * @file   spamd_lexer.h
 * @brief  Interface for spamd blacklist lexer.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef SPAMD_LEXER_DEFINED
#define SPAMD_LEXER_DEFINED

#include "lexer.h"

#define SPAMD_LEXER_MAX_INT   255

#define SPAMD_LEXER_TOK_DOT   100
#define SPAMD_LEXER_TOK_INT   101
#define SPAMD_LEXER_TOK_DASH  102
#define SPAMD_LEXER_TOK_SLASH 103
#define SPAMD_LEXER_TOK_EOF   104
#define SPAMD_LEXER_TOK_EOL   105

/**
 * Create a fresh lexer object.
 */
extern Lexer_T Spamd_lexer_create(Lexer_source_T source);

/**
 * Scan the specified lexer source's character stream and return
 * the next token seen.
 */
extern int Spamd_lexer_next_token(Lexer_T lexer);

#endif
