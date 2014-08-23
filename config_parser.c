/**
 * @file   config_parser.c
 * @brief  Implements the configuration file parser interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "utils.h"
#include "failures.h"
#include "config_parser.h"

#include <stdlib.h>
#include <string.h>

#define T Config_parser_T

/*
 * Parser utility/helper functions.
 */
static int  accept(T parser, int tok);
static int  accept_no_advance(T parser, int tok);
static void advance(T parser);

/*
 * Parser grammar functions.
 */
static int grammar_statement(T parser);
static int grammar_statements(T parser);
static int grammar_assignment(T parser);
static int grammar_list(T parser);
static int grammar_list_statements(T parser);
static int grammar_list_value(T parser);
static int grammar_list_values(T parser);
static int grammar_section(T parser);
static int grammar_section_statements(T parser);
static int grammar_section_assignments(T parser);
static int grammar_include(T parser);

extern T
Config_parser_create(Config_lexer_T lexer)
{
    T parser;

    if((parser = (T) malloc(sizeof(*parser))) == NULL) {
        I_CRIT("Could not create parser");
    }

    parser->lexer   = lexer;
    parser->config  = NULL;  /* Config reference set when the parser is started. */
    parser->section = NULL;  /* Section reference. */
    parser->value   = NULL;  /* Value reference. */

    return parser;
}

extern void
Config_parser_destroy(T parser)
{
    if(parser == NULL)
        return;

    if(parser->lexer != NULL) {
        Config_lexer_destroy(parser->lexer);
    }

    free(parser);
}

extern void
Config_parser_reset(T parser, Config_lexer_T lexer)
{
    /* Destroy the existing lexer. */
    if(parser->lexer) {
        Config_lexer_destroy(parser->lexer);
    }

    parser->lexer = lexer;
}

extern int
Config_parser_start(T parser, Config_T config)
{
    Config_section_T global_section;

    /* Store a reference to the config object. */
    parser->config = config;

    /* Ensure that there is a global config section available. */
    if((global_section = Config_get_section(config, CONFIG_PARSER_DEFAULT_SECTION)) == NULL) {
        global_section = Config_section_create(CONFIG_PARSER_DEFAULT_SECTION);
        Config_add_section(config, global_section);
    }

    /* Start token stream and consume all EOL tokens at the start of the file. */
    advance(parser);
    accept(parser, CONFIG_LEXER_TOK_EOL);

    /* Start the actual parsing. */
    if((grammar_statement(parser) && grammar_statements(parser) && accept(parser, CONFIG_LEXER_TOK_EOF))
       || accept(parser, CONFIG_LEXER_TOK_EOF))
    {
        return CONFIG_PARSER_OK;
    }

    return CONFIG_PARSER_ERR;
}

static int
accept_no_advance(T parser, int tok)
{
    return (tok == parser->curr);
}

static int
accept(T parser, int tok)
{
    if(accept_no_advance(parser, tok)) {
        /*
         * If accepted token is a EOL, accept all subsequent EOLs until the
         * first non-EOL token.
         */

        if(tok == CONFIG_LEXER_TOK_EOL) {
            do {
                advance(parser);
            }
            while(parser->curr == CONFIG_LEXER_TOK_EOL);
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
    parser->curr = Config_lexer_next_token(parser->lexer);
}

static int
grammar_statements(T parser)
{
    if(accept(parser, CONFIG_LEXER_TOK_EOL) && grammar_statement(parser) && grammar_statements(parser)) {
        return CONFIG_PARSER_OK;
    }

    /* Allow nothing here to complete the tail recursion. */
    return CONFIG_PARSER_OK;
}

static int
grammar_statement(T parser)
{
    if(grammar_assignment(parser)
       || grammar_section(parser)
       || grammar_include(parser))
    {
        return CONFIG_PARSER_OK;
    }

    return CONFIG_PARSER_ERR;
}

static int
grammar_assignment(T parser)
{
    char varname[CONFIG_LEXER_MAX_STR_LEN + 1];
    int len, isint = 0, isstr = 0;
    Config_section_T section;

    if(accept_no_advance(parser, CONFIG_LEXER_TOK_NAME)) {
        /* Store the variable name. */
        len = strlen(parser->lexer->current_value.s) + 1;
        sstrncpy(varname, parser->lexer->current_value.s, len);
        advance(parser);

        if(accept(parser, CONFIG_LEXER_TOK_EQ)
           && ((isint = accept_no_advance(parser, CONFIG_LEXER_TOK_INT))
               || (isstr = accept_no_advance(parser, CONFIG_LEXER_TOK_STR))
               || grammar_list(parser)))
        {
            /*
             * We have a complete assignment at this stage, so find the config section to
             * update with the new variable.
             */
            section = (parser->section ? parser->section
                       : Config_get_section(parser->config, CONFIG_PARSER_DEFAULT_SECTION));

            if(isint) {
                Config_section_set_int(section, varname, parser->lexer->current_value.i);
            }
            else if(isstr) {
                Config_section_set_str(section, varname, parser->lexer->current_value.s);
            }
            else {
                /*
                 * As this must be a list, the reference to the complete list is stored in the
                 * parser's reference to the list-type config value.
                 */
                Config_section_set(section, varname, parser->value);
                parser->value = NULL;
            }

            /* Continue the token stream. */
            if(isint || isstr)
                advance(parser);

            return CONFIG_PARSER_OK;
        }
    }

    return CONFIG_PARSER_ERR;
}

static int
grammar_list(T parser)
{
    if(accept(parser, CONFIG_LEXER_TOK_SQBRACK_L)) {
        /*
         * Setup the list config variable and save the reference.
         */
        parser->value = Config_value_create(CONFIG_VAL_TYPE_LIST);

        if(((accept(parser, CONFIG_LEXER_TOK_EOL) && grammar_list_statements(parser))
            || grammar_list_statements(parser))
           && ((accept(parser, CONFIG_LEXER_TOK_EOL) && accept(parser, CONFIG_LEXER_TOK_SQBRACK_R))
               || accept(parser, CONFIG_LEXER_TOK_SQBRACK_R)))
        {
            return CONFIG_PARSER_OK;
        }
    }

    return CONFIG_PARSER_ERR;
}

static int
grammar_list_statements(T parser)
{
    /*
     * There must be at least one list statement.
     */
    if(grammar_list_value(parser) && grammar_list_values(parser)) {
        return CONFIG_PARSER_OK;
    }

    return CONFIG_PARSER_ERR;
}

static int
grammar_list_value(T parser)
{
    Config_value_T value;

    if(accept_no_advance(parser, CONFIG_LEXER_TOK_INT)) {
        value = Config_value_create(CONFIG_VAL_TYPE_INT);
        Config_value_set_int(value, parser->lexer->current_value.i);
        advance(parser);
        List_insert_after(parser->value->v.l, (void *) value);

        return CONFIG_PARSER_OK;
    }
    else if(accept_no_advance(parser, CONFIG_LEXER_TOK_STR)) {
        value = Config_value_create(CONFIG_VAL_TYPE_STR);
        Config_value_set_str(value, parser->lexer->current_value.s);
        advance(parser);
        List_insert_after(parser->value->v.l, (void *) value);

        return CONFIG_PARSER_OK;
    }

    return CONFIG_PARSER_ERR;
}

static int
grammar_list_values(T parser)
{
    if(accept(parser, CONFIG_LEXER_TOK_COMMA)
       && ((accept(parser, CONFIG_LEXER_TOK_EOL) && grammar_list_value(parser) && grammar_list_values(parser))
           || (grammar_list_value(parser) && grammar_list_values(parser))))
    {
        return CONFIG_PARSER_OK;
    }

    /* Allow nothing here to complete the tail recursion. */
    return CONFIG_PARSER_OK;
}

static int
grammar_section(T parser)
{
    char secname[CONFIG_LEXER_MAX_STR_LEN + 1];
    int len;

    if(accept(parser, CONFIG_LEXER_TOK_SECTION) && accept_no_advance(parser, CONFIG_LEXER_TOK_NAME)) {
        /* Copy the section name. */
        len = strlen(parser->lexer->current_value.s) + 1;
        sstrncpy(secname, parser->lexer->current_value.s, len);
        advance(parser);

        if((accept(parser, CONFIG_LEXER_TOK_EOL) && accept(parser, CONFIG_LEXER_TOK_BRACKET_L)
           && accept(parser, CONFIG_LEXER_TOK_EOL))
           || (accept(parser, CONFIG_LEXER_TOK_BRACKET_L) && accept(parser, CONFIG_LEXER_TOK_EOL)))
        {
            /*
             * Create a new section and overwrite any existing sections of the same
             * name in the configuration.
             */
            parser->section = Config_section_create(secname);
            Config_add_section(parser->config, parser->section);

            if(grammar_section_statements(parser) && accept(parser, CONFIG_LEXER_TOK_EOL)
               && accept(parser, CONFIG_LEXER_TOK_BRACKET_R))
            {
                /*
                 * Remove the reference as we are now finished with this section.
                 */
                parser->section = NULL;

                return CONFIG_PARSER_OK;
            }
        }
    }

    return CONFIG_PARSER_ERR;
}

static int
grammar_section_statements(T parser)
{
    /*
     * There must be at least one statement.
     */
    if(grammar_assignment(parser) && grammar_section_assignments(parser)) {
        return CONFIG_PARSER_OK;
    }

    return CONFIG_PARSER_ERR;
}

static int
grammar_section_assignments(T parser)
{
    if(accept(parser, CONFIG_LEXER_TOK_COMMA) && accept(parser, CONFIG_LEXER_TOK_EOL)
       && grammar_assignment(parser) && grammar_section_assignments(parser))
    {
        return CONFIG_PARSER_OK;
    }

    /* Allow nothing here to complete the tail recursion. */
    return CONFIG_PARSER_OK;
}

static int
grammar_include(T parser)
{
    char *include;
    int len;

    if(accept(parser, CONFIG_LEXER_TOK_INCLUDE) && accept_no_advance(parser, CONFIG_LEXER_TOK_STR)) {
        /*
         * Enqueue the included file here and advance the token stream.
         */
        Config_add_include(parser->config, parser->lexer->current_value.s);
        advance(parser);

        return CONFIG_PARSER_OK;
    }

    return CONFIG_PARSER_ERR;
}

#undef T
