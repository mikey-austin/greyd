greyd.conf(5) -- greyd configuration file
=========================================

## SYNOPSIS

This configuration file is read by **greyd**, **greydb**, **greylogd** and **greyd-setup**.

## DESCRIPTION

The syntax consists of sequences of assignments, each terminated by a newline:

    # A string value.
    variable = "value"

    # A number value.
    variable = 10  # Another comment.

    # A list value may contain strings or numbers.
    # Trailing commas are allowed.
    variable = [ 10, "value", ]

Comments, whitespace and blank lines are ignored.

*Sections* may contain many assignments, separated by a newline.

    section sectionname {
        var1 = "val1"
        var2 = 10
        var3 = [ 1, 2, 3 ]
    }

*Blacklists* and *whitelists* use the same syntax as the *section* above (see [BLACKLIST CONFIGURATION][]):

    blacklist blacklistname {
        ...
    }

    whitelist whitelistname {
        ...
    }

Configuration may also be recursively loaded by way of an *include*:

    # Globbing is supported.
    include "/etc/greyd/conf.d/*.conf"

## GENERAL OPTIONS

The following options may be specified outside of a section. A *boolean* value is a *number* which takes the values *0* or *1*.

* **debug** = *boolean*:
  Log debug messages which are suppressed by default.

* **verbose** = *boolean*:
  Log blacklisted connection headers.

* **daemonize** = *boolean*:
  Detach from the controlling terminal. Defaults to *1*.

* **proxy_protocol_enable** = *boolean*:
  Proxy protocol configuration. Enabling this configuration allows greyd to sit behind a TCP load balancer that speaks the proxy protocol v1 as defined in the [protocol spec](http://www.haproxy.org/download/1.8/doc/proxy-protocol.txt).
  Defaults to *false*. Note that if this is enabled *all* client connections will need to specify the proxy protocol header first, ie there is no mixing of proxied and direct requests.
  You *must* also specify the `proxy_protocol_permitted_proxies` list of trusted proxies. There are many upstream proxies/load balancers that support this protocol, for example nginx and haproxy to name a couple.

* **proxy_protocol_permitted_proxies** = *list*:
  The upstream proxies must be explicitly configured. Without this any client would be able to spoof their addresses. This setting
  only has an effect if `proxy_protocol_enable` is set to *true*. The elements in this list must be strings consisting of IPv4 and/or IPv6
  CIDRs.

* **drop_privs** = *boolean*:
  Drop priviliges and run as the specified **user**. Defaults to *1*.

* **chroot** = *boolean*:
  Chroot the main **greyd** process that accepts connections. Defaults to *1*.

* **chroot_dir** = *string*:
  The location to chroot to.

* **setrlimit** = *boolean*:
  Use setrlimit to self-impose resource limits such as the maximum number of file descriptors (ie connections).

* **max_cons** = *number*:
  The maximum number of concurrent connections to handle. This number can not exceed the operating system maximum file descriptor limit. Defaults to *800*.

* **max_cons_black** = *number*:
  The maximum number of concurrent blacklisted connections to tarpit. This number can not exceed the maximum configured number of connections. Defaults to *800*.

* **port** = *number*:
  The port to listen on. Defaults to *8025*.

* **user** = *string*:
  The username for the main **greyd** daemon the run as.

* **bind_address** = *string*:
  The IPv4 address to listen on. Defaults to listen on all addresses.

* **port** = *number*:
  The port to listen on. Defaults to *8025*.

* **config_port** = *number*:
  The port on which to listen for blacklist configuration data (see **greyd-setup**(8)). Defaults to *8026*.

* **greyd_pidfile** = *string*:
  The greyd pidfile path.

* **greylogd_pidfile** = *string*:
  The greylogd pidfile path.

* **hostname** = *string*:
  The hostname to display to clients in the initial SMTP banner.

* **enable_ipv6** = *boolean*:
  Listen for IPv6 connections. Disabled by default.

* **bind_address_ipv6** = *string*:
  The IPv6 address to listen on. Only has an effect if **enable_ipv6** is set to true.

* **stutter** = *number*:
  For blacklisted connections, the number of seconds between stuttered bytes.

* **window** = *number*:
  Adjust the socket receive buffer to the specified number of bytes (window size). This slows down spammers even more.

* **banner** = *string*:
  The banner message to be displayed to new connections.

* **error_code** = *string*:
  The SMTP error code to show blacklisted spammers. May be either *"450"* (default) or *"550"*.

## FIREWALL SECTION

The following options are common to all firewall drivers:

* **driver** = *string*:
  The driver shared object location. May be either an absolute or relative path. If no '/' is present, then the system's dynamic linker will search the relevant paths. For example:

        section firewall {
            #driver = "greyd_pf.so"
            driver = "greyd_netfilter.so"

            # Driver-specific options below.
            ...
        }

### Netfilter firewall driver

This driver runs on GNU/Linux systems and makes use of *libipset*, *libnetfilter_conntrack* and *libnetfilter_log*.

* **max_elements** = *number*:
  Maximum number of ipset elements. Defaults to *200,000*.

* **hash_size** = *number*:
  Maximum ipset hash size for each set.

* **track_outbound** = *boolean*:
  Track outbound connections. See **greylogd**(8) for more details.

* **inbound_group** = *number*:
  The *--nflog-group* to indicate inbound SMTP connections.

* **outbound_group** = *number*:
  The *--nflog-group* to indicate outbound SMTP connections.

### PF firewall driver

This driver runs on BSD systems making use of the PF firewall. The driver makes use of *libpcap*.

* **pfdev_path** = *string*:
  Path to pfdev, defaults to */dev/pf*.

* **pfctl_path** = *string*:
  Path to pfctl utility, defaults to */sbin/pfctl*.

* **pflog_if** = *string*:
  Pflog interface to listen for logged packets, defaults to *pflog0*.

* **net_if** = *string*:
  Network interface to restrict monitored logged packets to. Not set by default.

## DATABASE SECTION

The following options are common to all database drivers:

* **driver** = *string*:
  The driver shared object location. May be either an absolute or relative path. If no '/' is present, then the system's dynamic linker will search the relevant paths. For example:

        section database {
            driver = "greyd_bdb.so"
            #driver = "greyd_bdb_sql.so"
            #driver = "greyd_sqlite.so"
            #driver = "greyd_mysql.so"

            # Driver-specific options below.
            ...
        }

### Berkeley DB database driver

The Berkeley database driver runs on all systems providing libdb version > 4 and is built using the **--with-bdb** configure option. On OpenBSD, the db4 port will need to be installed.

* **path** = *string*:
  The filesystem path to the Berkeley DB environment.

* **db_name** = *string*:
  The name of the database file, relative to the specified environment **path**.

### Berkeley DB SQL database driver

The Berkeley DB SQL driver makes use of libdb_sql, which is available in Berkeley DB versions >= 5.x. This driver is built by specifying the **--with-bdb-sql** configure option.

* **path** = *string*:
  The filesystem path to the directory containing the database files.

* **db_name** = *string*:
  The name of the database file, relative to the specified **path**.

### SQLite database driver

The SQLite database driver makes use of libsqlite3. No special initialization is required as the driver will manage the schema internally. This driver is built by specifying the **--with-sqlite** configure option.

* **path** = *string*:
  The filesystem path to the directory containing the database files.

* **db_name** = *string*:
  The name of the database file, relative to the specified **path**.

### MySQL database driver

The MySQL driver may be built by specifying the **--with-mysql** configure option. The desired database will need to be setup independently of *greyd* using the **mysql_schema.sql** DDL distributed with the source distribution.

* **host** = *string*:
  The database host. Defaults to *localhost*.

* **port** = *number*:
  The database port. Defaults to 3306.

* **name** = *string*:
  The database name. Defaults to *greyd*.

* **user** = *string*:
  The database username.

* **pass** = *string*:
  The database password.

* **socket** = *string*:
  The path to the UNIX domain socket.

## GREY SECTION

* **enable** = *boolean*:
  Enable/disable the greylisting engine. Defaults to *1*.

* **user** = *string*:
  The username to run as for the greylisting processes. Defaults to *greydb*. This should differ from the *user* that the main **greyd** process is running as.

* **traplist_name** = *string*:
  The name of the blacklist to which spamtrapped hosts are added.

* **traplist_message** = *string*:
  The blacklist rejection message. See the *message* field in [BLACKLIST CONFIGURATION][].

* **whitelist_name** = *string*:
  The firewall whitelist *set/table* name. Defaults to *greyd-whitelist*.

* **whitelist_name_ipv6** = *string*:
  The firewall whitelist *set/table* name for IPv6 hosts. Defaults to *greyd-whitelist-ipv6*.

* **low_prio_mx** = *string*:
  The address of the secondary MX server, to greytrap hosts attempting to deliver spam to the MX servers in the incorrect order.

* **stutter** = *number*:
  Kill stutter for new grey connections after so many seconds. Defaults to *10*.

* **permitted_domains** = *string*:
  Filesystem location of the domains allowed to receive mail. If this file is specified (and exists), any message received with a RCPT TO domain *not* matching an entry in the below file will be greytrapped (ie blacklisted).

* **db_permitted_domains** = *boolean*:
  Augment *permitted_domains* (or replace if *permitted_domains* is not set) with DOMAIN entries loaded into the database. See **greydb**(8) for more on managing these database permitted domains.

* **pass_time** = *number*:
  The amount of time in seconds after which to whitelist grey entries. Defaults to *25 minutes*.

* **grey_expiry** = *number*:
  The amount of time in seconds after which to remove grey entries. Defaults to *4 hours*.

* **white_expiry** = *number*:
  The amount of time in seconds after which to remove whitelisted entries. Defaults to *31 days*.

* **trap_expiry** = *number*:
  The amount of time in seconds after which to remove greytrapped entries. Defaults to *1 day*.

## SYNCHRONISATION SECTION

* **enable** = *boolean*:
  Enable/disable the synchronisation engine. Defaults to *0*.

* **hosts** = *list*:
  Specify a list of *sync targets*. See the **-Y** option in **greyd**(8).

* **bind_address** = *string*:
  See **-y** option in **greyd**(8).

* **port** = *number*:
  The port on which to listen for incoming UDP sync messages.

* **ttl** = *number*:
  Specify a multicast TTL value. Defaults to *1*.

* **verify** = *boolean*:
  Load the specified *key* for verifying sync messages.

* **key** = *string*:
  The filesystem path to the key used to verify sync messages.

* **mcast_address** = *string*:
  The multicast group address for sync messages.

## SPF SECTION

This section controls the operation of the SPF validation functionality. Use the **--with-spf** configure flag to compile in SPF support.

* **enable** = *boolean*:
  Enable the SPF checking functionality.

* **trap_on_softfail** = *boolean*:
  Trap a host producing an SPF softfail. SPF hardfails are always trapped.

* **whitelist_on_pass** = *boolean*:
  Whitelist a host which passes SPF validation. This is disabled by default.

## SETUP SECTION

This section controls the operation of the **greyd-setup**(8) program.

* **lists** = *list*:
  The list of blacklists/whitelists to load. The order is important, see [BLACKLIST CONFIGURATION][]. Consecutive blacklists will be merged, with overlapping regions removed. If a blacklist (or series of blacklists) is followed by a whitelist, any address appearing on both will be removed.

* **curl_path** = *string*:
  The path to the *curl* program, which is used to fetch the lists via *HTTP* and *FTP*.

* **curl_proxy** = *string*:
  Specify a *proxyhost[:port]* through which to fetch the lists.

## BLACKLIST CONFIGURATION

A blacklist must contain the following fields:

* **message** = *string*:
  The message to be sent to **greyd**(8). This message will be displayed to clients who are on this list.

* **method** = *string*:
  The method in which the list of addresses is fetched. This may be one of *http*, *ftp*, *exec* or *file*.

* **file** = *string*:
  The argument to the specified *method*. For example, if the *http* method is specified, the *file* refers to the URL (minus the protocol).

An example blacklist definition is as follows:

    blacklist nixspam {
        message = "Your address %A is in the nixspam list"
        method  = "http"
        file = "www.openbsd.org/spamd/nixspam.gz"
    }

### Whitelist definitions

Whitelist definitions take the same fields as a blacklist definition, with the exception of the *message* (which is not applicable). For example:

    whitelist work_clients {
        method = "exec"
        file = "cat /tmp/work-clients-traplist.gz"
    }

### Address format

The format of the list of addresses is expected to consist of one network block or address per line (optionally followed by a space and text that is ignored). Comment lines beginning with # are ignored. Network blocks may be specified in any of the formats as in the following example:

    # CIDR format
    192.168.20.0/24
    # A start - end range
    192.168.21.0 - 192.168.21.255
    # As a single IP address
    192.168.23.1

Note, currently only IPv4 addresses are supported.

## COPYRIGHT

**greyd** is Copyright (C) 2015 Mikey Austin (greyd.org)

## SEE ALSO

  **greyd**(8), **greyd-setup**(8), **greydb**(8), **greylogd**(8)
