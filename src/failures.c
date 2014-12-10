/**
 * @file   failures.c
 * @brief  Implements the failure handling interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "log.h"
#include "constants.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>

#define RETURN_CRIT 1
#define RETURN_ERR  2

extern void
i_critical(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_CRIT, msg, args);
    va_end(args);

    exit(RETURN_CRIT);
}

extern void
i_error(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_ERR, msg, args);
    va_end(args);

    exit(RETURN_ERR);
}

extern void
i_warning(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_WARNING, msg, args);
    va_end(args);
}

extern void
i_info(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_INFO, msg, args);
    va_end(args);
}

extern void
i_debug(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    Log_write(LOG_DEBUG, msg, args);
    va_end(args);
}
