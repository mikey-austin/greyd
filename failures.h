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

#define I_CRIT(msg, ...)\
    i_critical(__FILE__, __LINE__, msg, ##__VA_ARGS__)

#define I_ERR(msg, ...)\
    i_error(__FILE__, __LINE__, msg, ##__VA_ARGS__)

#define I_WARN(msg, ...)\
    i_warning(__FILE__, __LINE__, msg, ##__VA_ARGS__)

#define I_INFO(msg, ...)\
    i_info(__FILE__, __LINE__, msg, ##__VA_ARGS__)

#define I_DEBUG(msg, ...)\
    i_debug(__FILE__, __LINE__, msg, ##__VA_ARGS__)

/**
 * The critical info function will log it's message and exit.
 */
extern void i_critical(const char *file, const int line, const char *msg, ...);

/**
 * The error info function will log it's message and exit.
 */
extern void i_error(const char *file, const int line, const char *msg, ...);

/**
 * The warning info function will just log it's message.
 */
extern void i_warning(const char *file, const int line, const char *msg, ...);

/**
 * The info function will just log it's message.
 */
extern void i_info(const char *file, const int line, const char *msg, ...);

/**
 * The debug function will just log it's message.
 */
extern void i_debug(const char *file, const int line, const char *msg, ...);

#endif
