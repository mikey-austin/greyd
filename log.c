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

extern void
Log_setup(Config_T config)
{
    Log_debug = Config_get_int(config, "debug", NULL, 0);

    if(Config_get_int(config, "syslog_enable", NULL, 1))
    {
        Log_syslog = 1;
        openlog(PROG_NAME, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    }
}

extern void
Log_write(int severity, const char *msg, va_list args)
{
    va_list syslog_args;

    if(Log_debug || (!Log_debug && severity < LOG_DEBUG)) {
        if(Log_syslog)
            va_copy(syslog_args, args);

        /* Always write to console. */
        printf("%s[%d]: ", PROG_NAME, getpid());
        vprintf(msg, args);
        printf("\n");

        if(Log_syslog)
            vsyslog(severity, msg, syslog_args);
    }
}
