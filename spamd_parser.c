/**
 * @file   spamd_parser.c
 * @brief  Implements the spamd parser interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "spamd_parser.h"
#include "blacklist.h"
#include "ip.h"

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define T Spamd_parser_T

#define SPAMD_ADDR_START 1
#define SPAMD_ADDR_END   0

/*
 * Parser utility/helper functions.
 */
static int  tok_accept(T parser, int tok);
static int  tok_accept_no_advance(T parser, int tok);
static void advance(T parser);

/*
 * Parser grammar functions.
 */
static int grammar_entry(T parser);
static int grammar_entries(T parser);
static int grammar_address(T parser, int end);

extern T
Spamd_parser_create(Lexer_T lexer)
{
    T parser;

    if((parser = malloc(sizeof(*parser))) == NULL) {
        I_CRIT("Could not create parser");
    }

    parser->lexer   = lexer;

    return parser;
}

extern void
Spamd_parser_destroy(T *parser)
{
    if(parser == NULL || *parser == NULL)
        return;

    if((*parser)->lexer != NULL) {
        Lexer_destroy(&((*parser)->lexer));
    }

    free(*parser);
    *parser = NULL;
}

extern void
Spamd_parser_reset(T parser, Lexer_T lexer)
{
    /* Destroy the existing lexer. */
    if(parser->lexer) {
        Lexer_destroy(&(parser->lexer));
    }

    parser->lexer = lexer;
}

extern int
Spamd_parser_start(T parser, Blacklist_T blacklist, int type)
{
    /*
     * Store references to the list to be populated, along with
     * the type of entries.
     */
    parser->blacklist = blacklist;
    parser->type      = type;

    /* Get rid of all beginning new lines at the start of the feed. */
    advance(parser);
    tok_accept(parser, SPAMD_LEXER_TOK_EOL);

    /* Start the actual parsing. */
    if((grammar_entry(parser) && grammar_entries(parser)
        && tok_accept(parser, SPAMD_LEXER_TOK_EOF))
       || tok_accept(parser, SPAMD_LEXER_TOK_EOF))
    {
        return SPAMD_PARSER_OK;
    }

    return SPAMD_PARSER_ERR;
}

static int
grammar_entry(T parser)
{
    struct IP_cidr cidr;
    u_int32_t start, end;

    if(grammar_address(parser, SPAMD_ADDR_START)) {
        if(tok_accept(parser, SPAMD_LEXER_TOK_SLASH)) {
            if(tok_accept_no_advance(parser, SPAMD_LEXER_TOK_INT6)) {
                /*
                 * We have a CIDR netblock, cast the four 8-bit bytes
                 * into a single 32-bit address.
                 */
                cidr.addr = ntohl(*((u_int32_t *) parser->start));
                cidr.bits = parser->lexer->current_value.i;
                advance(parser);

                IP_cidr_to_range(&cidr, &start, &end);
                end += 1;
            }
            else {
                return SPAMD_PARSER_ERR;
            }
        }
        else if(tok_accept(parser, SPAMD_LEXER_TOK_DASH)) {
            if(grammar_address(parser, SPAMD_ADDR_END)) {
                /*
                 * We have an address range stored in the parser object.
                 */
                start = ntohl(*((u_int32_t *) parser->start));
                end   = ntohl(*((u_int32_t *) parser->end)) + 1;
            }
            else {
                return SPAMD_PARSER_ERR;
            }
        }
        else {
            /*
             * We have a single address.
             */
            start = ntohl(*((u_int32_t *) parser->start));
            end   = start + 1;
        }

        /*
         * Add the address range onto the appropriate blacklist.
         */
        Blacklist_add_range(parser->blacklist, start, end, parser->type);

        return SPAMD_PARSER_OK;
    }

    return SPAMD_PARSER_ERR;
}

static int
grammar_entries(T parser)
{
    if(tok_accept(parser, SPAMD_LEXER_TOK_EOL) && grammar_entry(parser)
       && grammar_entries(parser))
    {
        return SPAMD_PARSER_OK;
    }

    /* Allow nothing here to complete the tail recursion. */
    return SPAMD_PARSER_OK;
}

static int
grammar_address(T parser, int start)
{
    int i;

    for(i = 0; i < SPAMD_PARSER_IPV4_QUADS
            && (tok_accept_no_advance(parser, SPAMD_LEXER_TOK_INT6)
                || tok_accept_no_advance(parser, SPAMD_LEXER_TOK_INT8)); i++)
    {
        if(start) {
            parser->start[i] = parser->lexer->current_value.i;
        }
        else {
            parser->end[i] = parser->lexer->current_value.i;
        }
        advance(parser);

        if(i < (SPAMD_PARSER_IPV4_QUADS - 1)
           && !tok_accept(parser, SPAMD_LEXER_TOK_DOT))
        {
            return SPAMD_PARSER_ERR;
        }
    }

    /*
     * Return an error if we didn't receive a full address.
     */
    return (i == SPAMD_PARSER_IPV4_QUADS
            ? SPAMD_PARSER_OK : SPAMD_PARSER_ERR);
}

static int
tok_accept_no_advance(T parser, int tok)
{
    return (tok == parser->curr);
}

static int
tok_accept(T parser, int tok)
{
    if(tok_accept_no_advance(parser, tok)) {
        /*
         * If accepted token is a EOL, accept all subsequent EOLs until the
         * first non-EOL token.
         */

        if(tok == SPAMD_LEXER_TOK_EOL) {
            do {
                advance(parser);
            }
            while(parser->curr == SPAMD_LEXER_TOK_EOL);
        }
        else {
            advance(parser);
        }

        return 1;
    }

    return 0;
}

static void
advance(T parser)
{
    parser->curr = Lexer_next_token(parser->lexer);
}

#undef T
