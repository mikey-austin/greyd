/**
 * @file   firewall.h
 * @brief  Defines the interface for the firewall control.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef FIREWALL_DEFINED
#define FIREWALL_DEFINED

#include "config.h"
#include "list.h"

#include <arpa/inet.h>
#include <stdio.h>

typedef struct FW_handle_T *FW_handle_T;
struct FW_handle_T {
    void *fwh;
    void *driver;             /**< Driver dependent handle reference. */
    Config_T config;          /**< System configuration. */
    Config_section_T section; /**< Module configuration section. */

    int (*fw_open)(FW_handle_T);
    void (*fw_close)(FW_handle_T);
    int (*fw_replace)(FW_handle_T, const char *, List_T, short);
    void (*fw_init_log_capture)(FW_handle_T);
    void (*fw_end_log_capture)(FW_handle_T);
    struct FW_log_entry *(*fw_capture_log)(FW_handle_T);
    int (*fw_lookup_orig_dst)(FW_handle_T, struct sockaddr *,
                              struct sockaddr *, struct sockaddr *);
};

struct FW_log_entry {
    char *ipv4;
    char *ipv6;
};

/**
 * Open and initialize a firewall handle.
 */
extern FW_handle_T FW_open(Config_T config);

/**
 * Close the handle and invalidate pointers.
 */
extern void FW_close(FW_handle_T *handle);

/**
 * For the supplied IP set/table name, replace with the supplied list if
 * network blocks.
 */
extern int FW_replace(FW_handle_T handle, const char *set, List_T cidrs, short af);

/**
 * Initialize the log capture machinery.
 */
extern void FW_init_log_capture(FW_handle_T handle);

/**
 * Stop the log capture machinery and clean up.
 */
extern void FW_end_log_capture(FW_handle_T handle);

/**
 * Wait until a suitable log message arrives, then extract and return
 * the IP address (IPv4 or IPv6) as a NUL-terminated string.
 */
extern struct FW_log_entry *FW_capture_log(FW_handle_T handle);

/**
 * As connections are redirected to greyd by way of a DNAT, consult
 * the firewall connection tracking to lookup the original destination
 * (ie the destination address before the DNAT took place).
 */
extern int FW_lookup_orig_dst(FW_handle_T handle, struct sockaddr *src,
                              struct sockaddr *proxy, struct sockaddr *orig_dst);

#endif
