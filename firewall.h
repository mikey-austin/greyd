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
    int (*fw_log_capture_loop)(FW_handle_T, void (*callback)(char *, void *),
                               void *);
    int (*fw_lookup_orig_dst)(FW_handle_T, struct sockaddr *,
                              struct sockaddr *, struct sockaddr *);
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
 * Loop to receive messages from the firewall via it's internal mechanism, looking
 * for SMTP connections. When such a connection is witnessed, the supplied callback
 * will be called with the IP address to whitelist and the supplied arg as arguments.
 */
extern int FW_log_capture_loop(FW_handle_T handle, void (*callback)(char *, void *),
                               void *arg);

/**
 * As connections are redirected to greyd by way of a DNAT, consult
 * the firewall connection tracking to lookup the original destination
 * (ie the destination address before the DNAT took place).
 */
extern int FW_lookup_orig_dst(FW_handle_T handle, struct sockaddr *src,
                              struct sockaddr *proxy, struct sockaddr *orig_dst);

#endif
