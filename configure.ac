AC_PREREQ([2.68])
AC_INIT([greyd], [0.6.1], [mikey@greyd.org])
AM_INIT_AUTOMAKE
LT_INIT([dlopen])
AC_CONFIG_SRCDIR([src/main_greyd.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_REQUIRE_AUX_FILE([tap-driver.pl])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_SED
AM_PROG_CC_C_O

# Checks for common libraries.
AC_CHECK_LIB([ltdl], [lt_dlopen], [have_ltdl="yes"])
if test "x${have_ltdl}" = xyes; then
    AC_CHECK_HEADERS([ltdl.h], [], [have_ltdl="yes"], [have_ltdl="no"])
fi

if test "x${have_ltdl}" = xyes; then
    AC_DEFINE([HAVE_LTDL_H], [1], [libtool ltdl abstraction library])
    AC_SUBST([HAVE_LIBLTDL], "yes")
    LIBS="${LIBS} -lltdl"
else
    AC_MSG_FAILURE([ltdl is required along with its headers])
fi

AC_CHECK_LIB([z], [gzdopen], [have_gz="yes"])
if test "x${have_gz}" = xyes; then
    AC_CHECK_HEADERS([zlib.h], [], [have_gz="yes"], [have_gz="no"])
fi

if test "x${have_gz}" = xyes; then
    LIBS="${LIBS} -lz"
else
    AC_MSG_FAILURE([zlib is required along with its headers])
fi

AC_CHECK_LIB([crypto], [HMAC_Init], [have_crypto="yes"])
if test "x${have_crypto}" = xyes; then
    AC_CHECK_HEADERS([openssl/hmac.h openssl/sha1.h], [], [have_crypto="yes"], [have_crypto="no"])
fi

if test "x${have_crypto}" = xyes; then
    LIBS="${LIBS} -lcrypto"
else
    AC_MSG_FAILURE([libcrypto (OpenSSL) is missing, required for sync])
fi

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/socket.h syslog.h unistd.h])

# Optional drivers to build.
optional_drivers=
optional_ldadd=
AC_SUBST([optional_drivers])
AC_SUBST([optional_ldadd])

AC_ARG_VAR([DEFAULT_CONFIG], [default greyd configuration path])
if test -z ${DEFAULT_CONFIG}; then
    DEFAULT_CONFIG="${sysconfdir}/greyd/greyd.conf"
fi

AC_ARG_VAR([GREYD_PIDFILE], [default greyd pidfile path])
if test -z ${GREYD_PIDFILE}; then
    GREYD_PIDFILE="${localstatedir}/run/greyd.pid"
fi

AC_ARG_VAR([GREYLOGD_PIDFILE], [default greylogd pidfile path])
if test -z ${GREYLOGD_PIDFILE}; then
    GREYLOGD_PIDFILE="${localstatedir}/run/greylogd.pid"
fi

#
# Berkeley db driver library & header checks.
#
AC_ARG_WITH([bdb], [AS_HELP_STRING([--with-bdb], [build the Berkeley database driver])],
    [bdb_driver=yes], [bdb_driver=no])

if test "x${bdb_driver}" = xyes; then
    have_bdb=no
    AC_SEARCH_LIBS([db_env_create], [db-5.3 db5 db4 db], [have_bdb=yes])
    if test "x${have_bdb}" = xyes; then
        AC_CHECK_HEADERS([db5/db.h db4/db.h db.h], [have_bdb=yes;break], [have_bdb=no])
    fi

    if test "x${have_bdb}" = xyes; then
        AC_DEFINE([HAVE_LIBDB], [1], [Berkeley DB development library])
    else
        AC_MSG_FAILURE([libdb is required to build the Berkeley database driver])
    fi
fi

if test "x${bdb_driver}" = xyes; then
    AC_DEFINE([WITH_BDB], [1], [with the Berkeley DB driver])
    optional_drivers="${optional_drivers} greyd_bdb.la"
    optional_ldadd="${optional_ldadd} -dlopen ../drivers/greyd_bdb.la"
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
        AC_CHECK_HEADERS([sys/prctl.h], [], [have_netfilter=no])
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
    LIBS="${LIBS} ${netfilter_LIBS}"
    AC_DEFINE([WITH_NETFILTER], [1], [with the Netfilter firewall driver])
    optional_drivers="${optional_drivers} greyd_netfilter.la"
    optional_ldadd="${optional_ldadd} -dlopen ../drivers/greyd_netfilter.la"
fi

#
# PF firewall driver.
#
AC_ARG_WITH([pf], [AS_HELP_STRING([--with-pf], [build the PF firewall driver])],
    [pf_driver=yes], [pf_driver=no])

if test "x${pf_driver}" = xyes; then
    pf_LIBS=""
    have_pf=no
    AC_CHECK_LIB([pcap], [pcap_open_live], [have_pf=yes])
    if test "x${have_pf}" = xyes; then
        AC_CHECK_HEADERS([pcap.h], [], [have_pf=no])
    fi

    if test "x${have_pf}" = xyes; then
        pf_LIBS="$pf_LIBS -lpcap"
        AC_DEFINE([HAVE_LIBPCAP], [1], [Packet capture library])
        AC_SUBST([pf_LIBS], ["$pf_LIBS"])
        LIBS="${LIBS} ${pf_LIBS}"
    else
        AC_MSG_FAILURE([libpcap is required to build the pf driver])
    fi

    if test "x${have_pf}" = xyes; then
        AC_CHECK_HEADERS([net/pfvar.h net/pf/pfvar.h], [have_pf=yes;break], [have_pf=no],
        [[
         #include <sys/types.h>
         #include <sys/socket.h>
         #include <net/if.h>
         #include <netinet/in.h>
         ]])
    fi

    if test "x${have_pf}" != xyes; then
        AC_MSG_FAILURE([net/pfvar.h header missing])
    fi

    if test "x${have_pf}" = xyes; then
        AC_CHECK_HEADERS([net/if_pflog.h net/pf/if_pflog.h], [have_pf=yes;break], [have_pf=no],
        [[
         #include <sys/types.h>
         #include <sys/socket.h>
         #include <netinet/in.h>
         #include <net/if.h>
         ]])
    fi

    if test "x${have_pf}" != xyes; then
        AC_MSG_FAILURE([net/if_pflog.h header missing])
    fi

    AC_PATH_PROGS([PFCTL], [pfctl], [/sbin/pfctl], ["/sbin:$PATH"])

    AC_DEFINE([WITH_PF], [1], [with the PF firewall driver])
    optional_drivers="${optional_drivers} greyd_pf.la"
    optional_ldadd="${optional_ldadd} -dlopen ../drivers/greyd_pf.la"
fi

#
# NPF firewall driver.
#
AC_ARG_WITH([npf], [AS_HELP_STRING([--with-npf], [build the NPF firewall driver])],
    [npf_driver=yes], [npf_driver=no])

if test "x${npf_driver}" = xyes; then
    npf_LIBS=""
    have_npf=no
    AC_CHECK_LIB([pcap], [pcap_open_live], [have_npf=yes])
    if test "x${have_npf}" = xyes; then
        AC_CHECK_HEADERS([pcap.h], [], [have_npf=no])
    fi

    if test "x${have_npf}" = xyes; then
        npf_LIBS="$npf_LIBS -lpcap"
        AC_DEFINE([HAVE_LIBPCAP], [1], [Packet capture library])
        AC_SUBST([npf_LIBS], ["$npf_LIBS"])
        LIBS="${LIBS} ${npf_LIBS}"
    else
        AC_MSG_FAILURE([libpcap is required to build the npf driver])
    fi

    AC_CHECK_LIB([npf], [npf_parse_cidr], [have_npf=yes])
    if test "x${have_npf}" = xyes; then
        npf_LIBS="$npf_LIBS -lnpf"
        AC_DEFINE([HAVE_LIBNPF], [1], [NPF control library])
        AC_SUBST([npf_LIBS], ["$npf_LIBS"])
        LIBS="${LIBS} ${npf_LIBS}"
    else
        AC_MSG_FAILURE([libnpf is required to build the npf driver])
    fi


    AC_DEFINE([WITH_NPF], [1], [with the npf firewall driver])
    optional_drivers="${optional_drivers} greyd_npf.la"
    optional_ldadd="${optional_ldadd} -dlopen ../drivers/greyd_npf.la"
fi

#
# SPF support via libspf2.
#
AC_ARG_WITH([spf], [AS_HELP_STRING([--with-spf], [include spf support via libspf2])],
    [spf_module=yes], [spf_module=no])

if test "x${spf_module}" = xyes; then
    have_spf=no
    AC_SEARCH_LIBS([SPF_server_new], [spf2 spf], [have_spf=yes])
    if test "x${have_spf}" = xyes; then
        AC_CHECK_HEADERS([spf2/spf.h spf.h], [have_spf=yes;break], [have_spf=no],
        [[
         #include <time.h>
         #include <sys/socket.h>
         #include <netinet/in.h>
         #include <arpa/inet.h>
        ]])
    fi

    if test "x${have_spf}" = xyes; then
        AC_DEFINE([HAVE_SPF], [1], [spf functionality])
    else
        AC_MSG_FAILURE([libspf2 is required for spf support])
    fi
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
AC_CHECK_FUNCS([dup2 getcwd gethostname inet_ntoa memset mkdir socket strchr strdup strerror strncasecmp strpbrk strtol tzset strnlen setresgid setresuid setregid setreuid])

AC_CONFIG_FILES([Makefile
        src/Makefile
        drivers/Makefile
        doc/Makefile
        etc/Makefile
        check/Makefile
        check/run_tests.pl
        check/data/permitted_domains.txt
        check/data/config_lexer_test1.conf
        check/data/config_test1.conf
        check/data/config_test2.conf
        check/data/config_test3.conf
        check/data/lexer_source_1.conf])
AC_OUTPUT
