/**
 * @file   config_parser.h
 * @brief  Defines the configuration file parser interface.
 * @author Mikey Austin
 * @date   2014
 *
 * The config grammar is as follows (in BNF form, terminals uppercase):
 *
 * @code
 * config : statement EOF
 *        ;
 *
 * statement : assignment
 *           | section
 *           | include
 *           | statement
 *           ;
 *
 * assignment : NAME = INT
 *            | NAME = STR
 *            ;
 *
 * section : SECTION NAME { section_assignment }
 *         ;
 *
 * section_assignment : assignment
 *                    | section_assignment
 *                    ;
 *
 * include : INCLUDE STR
 *         ;
 * @endcode
 */

#ifndef CONFIG_PARSER_DEFINED
#define CONFIG_PARSER_DEFINED

#include "config_lexer.h"
#include "config.h"

#define T Config_parser_T

#define CONFIG_PARSER_OK  1
#define CONFIG_PARSER_ERR 0

/**
 * The main config parser structure.
 */
typedef struct T *T;
struct T {
    Config_T         config;  /**< Reference to the config object. */
    Config_section_T section; /**< Reference to the current section. */
    Config_lexer_T   lexer;
    int              curr;    /**< The current token being looked at. */
};

/**
 * Create a fresh parser object.
 *
 * @param lexer The initialized token stream.
 *
 * @return An initialized parser object.
 */
extern T Config_parser_create(Config_lexer_T lexer);

/**
 * Destroy a parser object. This will automatically destroy the associated
 * configuration source object.
 *
 * @param parser The initialized parser object to be destroyed.
 */
extern void Config_parser_destroy(T parser);

/**
 * Re-set the parser with a new token stream.
 *
 * @param parser The initialized parser object.
 * @param lexer The initialized token stream.
 */
extern void Config_parser_reset(T parser, Config_lexer_T lexer);

/**
 * Start parsing the lexer's token stream, and populate the specified config object.
 *
 * @param parser The initialized parser object.
 * @param config The initialized configuration object to populate.
 *
 * @return 1 if there are no problems, 0 otherwise.
 */
extern int Config_parser_start(T parser, Config_T config);

#undef T
#endif
