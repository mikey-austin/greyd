/**
 * @file   failures.h
 * @brief  Defines program-wide functions to handle failures.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef FAILURES_DEFINED
#define FAILURES_DEFINED

#include <stdarg.h>

#define I_CRIT(msg, ...)\
    i_critical(__FILE__, __LINE__, msg, __VA_ARGS__)

#define I_ERR(msg, ...)\
    i_error(__FILE__, __LINE__, msg, __VA_ARGS__)

#define I_WARN(msg, ...)\
    i_warning(__FILE__, __LINE__, msg, __VA_ARGS__)

#define I_INFO(msg, ...)\
    i_info(__FILE__, __LINE__, msg, __VA_ARGS__)

extern void i_critical(const char *file, const int line, const char *msg, ...);
extern void i_error(const char *file, const int line, const char *msg, ...);
extern void i_warning(const char *file, const int line, const char *msg, ...);
extern void i_info(const char *file, const int line, const char *msg, ...);
#endif
