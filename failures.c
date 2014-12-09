/**
 * @file   failures.c
 * @brief  Implements the failure handling interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "constants.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define SEV_CRIT  "CRITICAL"
#define SEV_ERR   "ERROR"
#define SEV_WARN  "WARNING"
#define SEV_INFO  "INFO"
#define SEV_DEBUG "DEBUG"

#define RETURN_CRIT 1
#define RETURN_ERR  2

static void vlog(const char *severity, const char *msg, va_list args);

extern void
i_critical(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_CRIT, msg, args);
    va_end(args);

    exit(RETURN_CRIT);
}

extern void
i_error(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_ERR, msg, args);
    va_end(args);

    exit(RETURN_ERR);
}

extern void
i_warning(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_WARN, msg, args);
    va_end(args);
}

extern void
i_info(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_INFO, msg, args);
    va_end(args);
}

extern void
i_debug(const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_DEBUG, msg, args);
    va_end(args);
}

void
vlog(const char *severity, const char *msg, va_list args)
{
    printf("%s[%d]: ", PROG_NAME, getpid());
    vprintf(msg, args);
    printf(" (%s)\n", severity);
}
