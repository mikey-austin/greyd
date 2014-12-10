/**
 * @file   failures.h
 * @brief  Defines program-wide functions to handle failures.
 * @author Mikey Austin
 * @date   2014
 *
 * All of the error handling functions below log/print their messages, and
 * may exit depending on their severity.
 */

#ifndef FAILURES_DEFINED
#define FAILURES_DEFINED

#include <stdarg.h>

/**
 * The critical info function will log it's message and exit.
 */
extern void i_critical(const char *msg, ...);

/**
 * The error info function will log it's message and exit.
 */
extern void i_error(const char *msg, ...);

/**
 * The warning info function will just log it's message.
 */
extern void i_warning(const char *msg, ...);

/**
 * The info function will just log it's message.
 */
extern void i_info(const char *msg, ...);

/**
 * The debug function will just log it's message.
 */
extern void i_debug(const char *msg, ...);

#endif
