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

#include <stdio.h>

typedef struct FW_handle_T *FW_handle_T;
struct FW_handle_T {
    void *fwh;
    void *driver;             /**< Driver dependent handle reference. */
    Config_T config;          /**< System configuration. */
    Config_section_T section; /**< Module configuration section. */

    void (*fw_open)(FW_handle_T handle);
    void (*fw_close)(FW_handle_T handle);
    int (*fw_replace)(FW_handle_T handle, const char *set_name, List_T cidrs);
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
extern int FW_replace(FW_handle_T handle, const char *set, List_T cidrs);

/**
 * Setup a pipe for communication with a firewall control command.
 */
extern FILE *FW_setup_cntl_pipe(char *command, char **argv);

#endif
