/**
 * @file   spamd_parser.h
 * @brief  Defines the spamd-style backlist/whitelist parser interface.
 * @author Mikey Austin
 * @date   2014
 *
 * @code
 * blacklist : entry entries EOF
 *           | EOF
 *           ;
 *
 * entries : EOL entry entries
 *         |
 *         ;
 *
 * entry : address
 *       | address / INT6
 *       | address - address
 *       ;
 *
 * address : number . number . number . number
 *         ;
 *
 * number : INT6
 *        | INT8
 *        ;
 * @endcode
 */

#ifndef SPAMD_PARSER_DEFINED
#define SPAMD_PARSER_DEFINED

#include "spamd_lexer.h"
#include "blacklist.h"
#include "ip.h"

#include <stdint.h>

#define T Spamd_parser_T

#define SPAMD_PARSER_OK  1
#define SPAMD_PARSER_ERR 0

#define SPAMD_PARSER_IPV4_QUADS 4

/**
 * The main spamd parser structure.
 */
typedef struct T *T;
struct T {
    u_int8_t    start[SPAMD_PARSER_IPV4_QUADS];
    u_int8_t    end[SPAMD_PARSER_IPV4_QUADS];
    Lexer_T     lexer;
    Blacklist_T blacklist;
    int         type;
    int         curr;
};

/**
 * Create a fresh parser object.
 *
 * @param lexer The initialized token stream.
 *
 * @return An initialized parser object.
 */
extern T Spamd_parser_create(Lexer_T lexer);

/**
 * Destroy a parser object.
 *
 * @param parser The initialized parser object to be destroyed.
 */
extern void Spamd_parser_destroy(T *parser);

/**
 * Start parsing the lexer's token stream, and populate the specified
 * blacklist object.
 *
 * @param parser The initialized parser object.
 * @param config The initialized configuration object to populate.
 *
 * @return 1 if there are no problems, 0 otherwise.
 */
extern int Spamd_parser_start(T parser, Blacklist_T blacklist, int type);

#undef T
#endif
