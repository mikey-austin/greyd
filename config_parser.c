/**
 * @file   config_parser.c
 * @brief  Implements the configuration file parser interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "config_parser.h"

#include <stdlib.h>

#define T Config_parser_T

extern T
Config_parser_create(Config_lexer_T lexer)
{
    T parser;

    if((parser = (T) malloc(sizeof(*parser))) == NULL) {
        I_CRIT("Could not create parser");
    }

    parser->lexer  = lexer;
    parser->config = NULL;  /* Config reference set when the parser is started. */

    return parser;
}

extern void
Config_parser_destroy(T parser)
{
    if(parser == NULL)
        return;

    if(parser->lexer != NULL)
        Config_lexer_destroy(parser->lexer);

    free(parser);
}

extern void
Config_parser_reset(T parser, Config_lexer_T lexer)
{
    /* Destroy the existing lexer. */
    if(parser->lexer)
        Config_lexer_destroy(parser->lexer);

    parser->lexer = lexer;
}

extern int
Config_parser_start(T parser, Config_T config)
{
    /* Store a reference to the config object. */
    parser->config = config;

    return CONFIG_PARSER_ERR;
}

#undef T
