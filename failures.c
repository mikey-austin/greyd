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

#define SEV_CRIT "CRITICAL"
#define SEV_ERR  "ERROR"
#define SEV_WARN "WARNING"
#define SEV_INFO "INFO"

#define RETURN_CRIT 1
#define RETURN_ERR  2

static void vlog(const char *severity, const char *file, const int line, const char *msg, va_list args);

extern void
i_critical(const char *file, const int line, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_CRIT, file, line, msg, args);
    va_end(args);

    exit(RETURN_CRIT);
}

extern void
i_error(const char *file, const int line, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_ERR, file, line, msg, args);
    va_end(args);

    exit(RETURN_ERR);
}

extern void
i_warning(const char *file, const int line, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_WARN, file, line, msg, args);
    va_end(args);
}

extern void
i_info(const char *file, const int line, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    vlog(SEV_INFO, file, line, msg, args);
    va_end(args);
}

void
vlog(const char *severity, const char *file, const int line, const char *msg, va_list args)
{
    printf("%s: %s logged at file %s line %d:\n  >> ", PROG_NAME, severity, file, line);
    vprintf(msg, args);
    printf("\n");
}
