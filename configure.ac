AC_PREREQ([2.69])
AC_INIT([greyd], [0.1.0], [bugs@greyd.org])
AM_INIT_AUTOMAKE([parallel-tests])
AC_CONFIG_SRCDIR([src/main_greyd.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_SED
AC_CHECK_PROGS([VALGRIND], [valgrind], )

# Checks for common libraries.
AC_CHECK_LIB([z], [gzdopen], [have_gz="yes"])
if test "x${have_gz}" = xyes; then
    AC_CHECK_HEADERS([zlib.h], [], [have_gz="yes"])
fi

if test "x${have_gz}" = xyes; then
   LIBS="$LIBS -lz"
else
    AC_MSG_FAILURE([zlib is required along with its headers])
fi

AC_CHECK_HEADERS([dlfcn.h], [have_dl="yes"])
if test "x${have_dl}" = xyes; then
    # Not all systems require -ldl (eg OpenBSD).
    AC_CHECK_LIB([dl], [dlopen], [LIBS="$LIBS -ldl"])
else
   AC_MSG_FAILURE([dynamic loader support is missing])
fi

AC_CHECK_LIB([crypto], [HMAC_Init], [have_crypto="yes"])
if test "x${have_crypto}" = xyes; then
    AC_CHECK_HEADERS([openssl/hmac.h openssl/sha1.h], [], [have_crypto="yes"])
fi

if test "x${have_crypto}" = xyes; then
   LIBS="$LIBS -lcrypto"
else
    AC_MSG_FAILURE([libcrypto (OpenSSL) is missing, required for sync])
fi

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/socket.h syslog.h unistd.h])

#
# Berkeley db driver library & header checks.
#
AC_ARG_WITH([bdb], [AS_HELP_STRING([--with-bdb], [build the Berkeley database driver])],
    [bdb_driver=yes], [bdb_driver=no])

if test "x${bdb_driver}" = xyes; then
    bdb_LIBS=""
    have_bdb=no
    AC_CHECK_LIB([db], [db_create], [have_bdb=yes])
    if test "x${have_bdb}" = xyes; then
       AC_CHECK_HEADERS([db.h], [], [have_bdb=no])
    fi

    if test "x${have_bdb}" = xyes; then
       bdb_LIBS="$bdb_LIBS -ldb"
       AC_DEFINE([HAVE_LIBDB], [1], [Berkeley DB development library])
       AC_SUBST([bdb_LIBS], ["$bdb_LIBS"])
    else
       AC_MSG_FAILURE([libdb is required to build the Berkeley database driver])
    fi
fi

if test "x${bdb_driver}" = xyes; then
   AC_DEFINE([WITH_BDB], [1], [with the Berkeley DB driver])
   AC_SUBST([WITH_BDB], [yes])
fi

#
# Netfilter driver library & header checks.
#
AC_ARG_WITH([netfilter], [AS_HELP_STRING([--with-netfilter], [build the Netfilter firewall driver])],
    [netfilter_driver=yes], [netfilter_driver=no])

if test "x${netfilter_driver}" = xyes; then
    netfilter_LIBS=""
    have_netfilter=no
    AC_CHECK_LIB([cap], [cap_get_proc], [have_netfilter=yes])
    if test "x${have_netfilter}" = xyes; then
       AC_CHECK_HEADERS([sys/capability.h], [], [have_netfilter=no])
    fi

    if test "x${have_netfilter}" = xyes; then
       netfilter_LIBS="$netfilter_LIBS -lcap"
       AC_DEFINE([HAVE_LIBCAP], [1], [Linux system capabilities])
    else
       AC_MSG_FAILURE([libcap is required to build the netfilter driver])
    fi

    AC_CHECK_LIB([ipset], [ipset_cmd], [have_netfilter=yes])
    if test "x${have_netfilter}" = xyes; then
       AC_CHECK_HEADERS([libipset/session.h], [], [have_netfilter=no])
    fi

    if test "x${have_netfilter}" = xyes; then
       netfilter_LIBS="$netfilter_LIBS -lipset"
       AC_DEFINE([HAVE_LIBIPSET], [1], [Netfilter IPSET management library])
    else
       AC_MSG_FAILURE([libipset is required to build the netfilter driver])
    fi

    AC_CHECK_LIB([mnl], [mnl_socket_open], [have_netfilter=yes])
    if test "x${have_netfilter}" = xyes; then
       AC_CHECK_HEADERS([libmnl/libmnl.h], [], [have_netfilter=no])
    fi

    if test "x${have_netfilter}" = xyes; then
       netfilter_LIBS="$netfilter_LIBS -lmnl"
       AC_DEFINE([HAVE_LIBMNL], [1], [Netlink socket abstraction library])
    else
       AC_MSG_FAILURE([libmnl is required to build the netfilter driver])
    fi

    AC_CHECK_LIB([netfilter_conntrack], [nfct_new], [have_netfilter=yes])
    if test "x${have_netfilter}" = xyes; then
       AC_CHECK_HEADERS([libnetfilter_conntrack/libnetfilter_conntrack.h], [], [have_netfilter=no])
    fi

    if test "x${have_netfilter}" = xyes; then
       netfilter_LIBS="$netfilter_LIBS -lnetfilter_conntrack"
       AC_DEFINE([HAVE_LIBNETFILTER_CONNTRACK], [1], [Netfilter connection tracking library])
    else
       AC_MSG_FAILURE([libnetfilter_conntrack is required to build the netfilter driver])
    fi

    AC_CHECK_LIB([netfilter_log], [nflog_open], [have_netfilter=yes])
    if test "x${have_netfilter}" = xyes; then
       AC_CHECK_HEADERS([libnetfilter_log/libnetfilter_log.h], [], [have_netfilter=no])
    fi

    if test "x${have_netfilter}" = xyes; then
       netfilter_LIBS="$netfilter_LIBS -lnetfilter_log"
       AC_DEFINE([HAVE_LIBNETFILTER_LOG], [1], [Netfilter connection tracking library])
    else
       AC_MSG_FAILURE([libnetfilter_log is required to build the netfilter driver])
    fi

    if test "x${have_netfilter}" = xyes; then
       AC_SUBST([netfilter_LIBS], ["$netfilter_LIBS"])
    fi
fi

if test "x${netfilter_driver}" = xyes; then
   AC_DEFINE([WITH_NETFILTER], [1], [with the Netfilter firewall driver])
   AC_SUBST([WITH_NETFILTER], [yes])
fi

#
# PF firewall driver.
#
AC_ARG_WITH([pf], [AS_HELP_STRING([--with-pf], [build the PF firewall driver])],
    [pf_driver=yes], [pf_driver=no])

if test "x${pf_driver}" = xyes; then
   AC_DEFINE([WITH_PF], [1], [with the PF firewall driver])
   AC_SUBST([WITH_PF], [yes])
fi

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
#AC_FUNC_STRNLEN
AC_CHECK_FUNCS([dup2 getcwd gethostname inet_ntoa memset mkdir socket strchr strdup strerror strncasecmp strpbrk strtol tzset strnlen])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/modules/Makefile
                 etc/Makefile
                 tests/Makefile
                 tests/run_tests.pl
                 tests/data/permitted_domains.txt
                 tests/data/config_lexer_test1.conf
                 tests/data/config_test1.conf
                 tests/data/config_test2.conf
                 tests/data/config_test3.conf
                 tests/data/lexer_source_1.conf
                 tests/data/lexer_source_2.conf.gz])
AC_OUTPUT
