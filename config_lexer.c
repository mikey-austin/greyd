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

#define T Config_lexer_T

#define L_GETC(lexer)      Lexer_source_getc(lexer->source)
#define L_UNGETC(lexer, c) Lexer_source_ungetc(lexer->source, c)
#define L_MATCH(a, len, b) (len == strlen(b) && strncmp(a, b, len) == 0)

/**
 * Put the character back onto the front of the queue while checking
 * for the end of the stream.
 */
static void reuse_char(T lexer, int c);

extern T
Config_lexer_create(Lexer_source_T source)
{
    T lexer;

    if((lexer = (T) malloc(sizeof(*lexer))) == NULL) {
        I_CRIT("Could not create lexer");
    }

    lexer->source   = source;
    lexer->seen_end = 0;

    return lexer;
}

extern void
Config_lexer_destroy(T lexer)
{
    if(!lexer)
        return;

    if(lexer->source)
        Lexer_source_destroy(lexer->source);

    free(lexer);
}

extern int
Config_lexer_next_token(T lexer)
{
    int c, seen_esc = 0, len = 0, i, tmp_tok;
    union Config_lexer_token_value tmp_val;

    /*
     * Scan for the next token.
     */
    for(;;) {
        if(lexer->seen_end) {
            return CONFIG_LEXER_TOK_EOF;
        }

        c = L_GETC(lexer);

        if(isdigit(c)) {
            /*
             * This is an integer.
             */

            for(i = (c - '0'); isdigit(c = L_GETC(lexer)); ) {
                i = (i * 10) + (c - '0');
            }

            lexer->current_value.i = i;
            reuse_char(lexer, c);

            return (lexer->current_token = CONFIG_LEXER_TOK_INT);
        }

        if(isalpha(c)) {
            /*
             * This is either a value name or a reserved word.
             */

            do {
                lexer->current_value.s[len++] = tolower(c);
            }
            while((isalpha(c = L_GETC(lexer)) || isdigit(c) || c == '_')
                  && len <= CONFIG_LEXER_MAX_STR_LEN);

            lexer->current_value.s[len] = '\0';
            reuse_char(lexer, c);

            /* Search for reserved words. */
            if(L_MATCH(lexer->current_value.s, len, "section")) {
                return (lexer->current_token = CONFIG_LEXER_TOK_SECTION);
            }
            else if(L_MATCH(lexer->current_value.s, len, "include")) {
                return (lexer->current_token = CONFIG_LEXER_TOK_INCLUDE);
            }
            else if(L_MATCH(lexer->current_value.s, len, "blacklist")) {
                return (lexer->current_token = CONFIG_LEXER_TOK_BLACKLIST);
            }
            else if(L_MATCH(lexer->current_value.s, len, "whitelist")) {
                return (lexer->current_token = CONFIG_LEXER_TOK_WHITELIST);
            }
            else {
                /*
                 * The scanned token is not a reserved word and is not a
                 * string value, so it must be a configuration variable name.
                 */

                return (lexer->current_token = CONFIG_LEXER_TOK_NAME);
            }
        }

        /*
         * Process remaining characters.
         */
        switch(c) {
        case ' ':
        case '\t':
        case '\r':
            /*
             * Ignore whitespace.
             */
            break;

        case '#':
            /*
             * Ignore everything until the end of the line (or end of file).
             */

            while((c = L_GETC(lexer)) != '\n' && c != EOF)
                ;
            reuse_char(lexer, c);

            continue;

        case '"':
            /*
             * As we are in a string, populate current string value, while accounting for escaped
             * quote characters.
             */

            while(((c = L_GETC(lexer)) != '"' || seen_esc) && len <= CONFIG_LEXER_MAX_STR_LEN && c != EOF) {
                if(c == '\\' && !seen_esc) {
                    seen_esc = 1;
                } else {
                    /* Add the character to the current value if there is space. */
                    lexer->current_value.s[len++] = c;

                    /* Re-set the escape flag. */
                    seen_esc = 0;
                }
            }

            if(c != '"') {
                /* We don't need the closing quote character. */
                reuse_char(lexer, c);
            }

            /* Close off the string. */
            lexer->current_value.s[len] = '\0';

            return (lexer->current_token = CONFIG_LEXER_TOK_STR);

        case '\n':
            return (lexer->current_token = CONFIG_LEXER_TOK_EOL);

        case ',':
            return (lexer->current_token = CONFIG_LEXER_TOK_COMMA);

        case '=':
            return (lexer->current_token = CONFIG_LEXER_TOK_EQ);

        case '{':
            return (lexer->current_token = CONFIG_LEXER_TOK_BRACKET_L);

        case '}':
            return (lexer->current_token = CONFIG_LEXER_TOK_BRACKET_R);

        case '[':
            return (lexer->current_token = CONFIG_LEXER_TOK_SQBRACK_L);

        case ']':
            return (lexer->current_token = CONFIG_LEXER_TOK_SQBRACK_R);

        default:
            /* Unknown character. */
            return CONFIG_LEXER_TOK_EOF;
        }
    }
}

static void
reuse_char(T lexer, int c)
{
    if(c == EOF || c < 0) {
        /* We can't reuse an EOF character. */
        lexer->seen_end = 1;
    }
    else {
        L_UNGETC(lexer, c);
    }
}

#undef T
