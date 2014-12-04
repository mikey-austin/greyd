/**
 * @file   constants.h
 * @brief  Defines program-wide constants.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CONSTANTS_DEFINED
#define CONSTANTS_DEFINED

#define PROG_NAME           "greyd"
#define GREYLISTING_ENABLED 1
#define IPV6_ENABLED        0
#define DEFAULT_CONFIG      "/etc/greyd/greyd.conf"
#define MAX_HOST_NAME       255
#define GREYD_PORT          8025
#define GREYD_SYNC_PORT     8025
#define GREYD_CFG_PORT      8026
#define GREYD_MAIN_USER     "greyd"
#define GREYD_DB_USER       "greydb"
#define GREYD_CHROOT        1
#define GREYD_CHROOT_DIR    "/var/empty"
#define GREYD_BACKLOG       10
#define MAX_FILES_THRESHOLD 200

#endif
