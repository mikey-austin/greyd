/**
 * @file   config_parser.h
 * @brief  Defines the configuration file parser interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONFIG_PARSER_DEFINED
#define CONFIG_PARSER_DEFINED

#include "config_lexer.h"
#include "config.h"

#define T Config_parser_T

#define CONFIG_PARSER_OK  0
#define CONFIG_PARSER_ERR 1

/**
 * The main config parser structure.
 */
typedef struct T *T;
struct T {
    Config_T       config; /**< Reference to the config object being populated. */
    Config_lexer_T lexer;
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
 * @return 0 if there are no problems, > 0 otherwise.
 */
extern int Config_parser_start(T parser, Config_T config);

#undef T
#endif
