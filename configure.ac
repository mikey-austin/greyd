AC_PREREQ([2.69])
AC_INIT([greyd], [0.1.0], [bugs@greyd.org])
AC_CONFIG_SRCDIR([src/main_greyd.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB([cap], [cap_get_proc])
AC_CHECK_LIB([cap], [cap_set_flag])
AC_CHECK_LIB([cap], [cap_set_proc])
AC_CHECK_LIB([cap], [cap_free])
AC_CHECK_LIB([cap], [cap_clear])
AC_CHECK_LIB([crypto], [HMAC_Init])
AC_CHECK_LIB([crypto], [HMAC_Update])
AC_CHECK_LIB([crypto], [HMAC_Final])
AC_CHECK_LIB([crypto], [HMAC_CTX_cleanup])
AC_CHECK_LIB([crypto], [HMAC_CTX_init])
AC_CHECK_LIB([crypto], [SHA1_Init])
AC_CHECK_LIB([crypto], [SHA1_Update])
AC_CHECK_LIB([crypto], [SHA1_Final])
AC_CHECK_LIB([db], [db_create])
AC_CHECK_LIB([db], [db_strerror])
AC_CHECK_LIB([db], [db_env_create])
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([dl], [dlerror])
AC_CHECK_LIB([dl], [dlclose])
AC_CHECK_LIB([dl], [dlsym])
AC_CHECK_LIB([ipset], [ipset_load_types])
AC_CHECK_LIB([ipset], [ipset_session_init])
AC_CHECK_LIB([ipset], [ipset_session_fini])
AC_CHECK_LIB([ipset], [ipset_session_data_set])
AC_CHECK_LIB([ipset], [ipset_session_error])
AC_CHECK_LIB([ipset], [ipset_type_get])
AC_CHECK_LIB([ipset], [ipset_cmd])
AC_CHECK_LIB([ipset], [ipset_envopt_parse])
AC_CHECK_LIB([mnl], [mnl_socket_open])
AC_CHECK_LIB([mnl], [mnl_socket_bind])
AC_CHECK_LIB([mnl], [mnl_socket_close])
AC_CHECK_LIB([mnl], [mnl_socket_get_portid])
AC_CHECK_LIB([mnl], [mnl_nlmsg_put_header])
AC_CHECK_LIB([mnl], [mnl_mlmsg_put_extra_header])
AC_CHECK_LIB([mnl], [mnl_socket_sendto])
AC_CHECK_LIB([mnl], [mnl_socket_recv])
AC_CHECK_LIB([mnl], [mnl_cb_run])
AC_CHECK_LIB([netfilter_conntrack], [nfct_new])
AC_CHECK_LIB([netfilter_conntrack], [nfct_set_attr_u8])
AC_CHECK_LIB([netfilter_conntrack], [nfct_set_attr_u16])
AC_CHECK_LIB([netfilter_conntrack], [nfct_set_attr_u32])
AC_CHECK_LIB([netfilter_conntrack], [nfct_set_attr])
AC_CHECK_LIB([netfilter_conntrack], [nfct_nlmsg_build])
AC_CHECK_LIB([netfilter_conntrack], [nfct_destroy])
AC_CHECK_LIB([netfilter_conntrack], [nfct_nlmsg_parse])
AC_CHECK_LIB([netfilter_conntrack], [nfct_get_attr_u8])
AC_CHECK_LIB([netfilter_conntrack], [nfct_get_attr_u16])
AC_CHECK_LIB([netfilter_conntrack], [nfct_get_attr_u32])
AC_CHECK_LIB([netfilter_conntrack], [nfct_get_attr])
AC_CHECK_LIB([netfilter_conntrack], [nfct_destroy])
AC_CHECK_LIB([netfilter_log], [nflog_handle])
AC_CHECK_LIB([netfilter_log], [nflog_group])
AC_CHECK_LIB([netfilter_log], [nflog_callback_register])
AC_CHECK_LIB([netfilter_log], [nflog_unbind_group])
AC_CHECK_LIB([netfilter_log], [nflog_close])
AC_CHECK_LIB([netfilter_log], [nflog_fd])
AC_CHECK_LIB([netfilter_log], [nflog_handle_packet])
AC_CHECK_LIB([netfilter_log], [nflog_open])
AC_CHECK_LIB([netfilter_log], [nflog_bind_pf])
AC_CHECK_LIB([netfilter_log], [nflog_set_mode])
AC_CHECK_LIB([netfilter_log], [nflog_set_nlbufsiz])
AC_CHECK_LIB([netfilter_log], [nflog_set_timeout])
AC_CHECK_LIB([netfilter_log], [nflog_get_payload])
AC_CHECK_LIB([z], [gzdopen])
AC_CHECK_LIB([z], [gzclose])
AC_CHECK_LIB([z], [gzgetc])
AC_CHECK_LIB([z], [gzungetc])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([dup2 getcwd gethostname inet_ntoa memset mkdir socket strchr strdup strerror strncasecmp strpbrk strtol tzset])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/modules/Makefile
                 tests/Makefile
                 etc/greyd.conf
                 README.md])
AC_OUTPUT
