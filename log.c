/**
 * @file   log.c
 * @brief  Logging interface implementation.
 * @author Mikey Austin
 * @date   2014
 */

#include "config.h"
#include "constants.h"

#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>
#include <unistd.h>

static short Log_debug  = 0;
static short Log_syslog = 0;
static const char *Log_ident = NULL;

extern void
Log_setup(Config_T config, const char *prog_name)
{
    Log_ident = prog_name;
    Log_debug = Config_get_int(config, "debug", NULL, 0);

    if(Config_get_int(config, "syslog_enable", NULL, 1))
    {
        Log_syslog = 1;
        openlog(Log_ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    }
}

extern void
Log_write(int severity, const char *msg, va_list args)
{
    va_list syslog_args;

    if(Log_debug || (!Log_debug && severity < LOG_DEBUG)) {
        if(Log_syslog) {
            va_copy(syslog_args, args);
            vsyslog(severity, msg, syslog_args);
            va_end(syslog_args);
        }

        /* Always write to console. */
        printf("%s[%d]: ", Log_ident, getpid());
        vprintf(msg, args);
        printf("\n");
    }
}
