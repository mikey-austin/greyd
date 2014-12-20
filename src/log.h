/**
 * @file   log.h
 * @brief  Logging interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "greyd_config.h"

#include <stdarg.h>

/**
 * Setup the logging framework.
 */
extern void Log_setup(Config_T config, const char *prog_name);

/**
 * Write a message to the logging system.
 */
extern void Log_write(int severity, const char *msg, va_list args);
