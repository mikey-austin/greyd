greyd(8) -- spam deferral daemon
=================================

## SYNOPSIS

`greyd` [**-456bdv**] [**-f** config] [**-B** maxblack] [**-c** maxcon] [**-G** passtime:greyexp:whiteexp] [**-h** hostname] [**-l** address] [**-L** address] [**-M** address] [**-n** name] [**-p** port] [**-S** secs] [**-s** secs] [**-w** window] [**-Y** synctarget] [**-y** synclisten]

## DESCRIPTION

**greyd** is a fake mail daemon which rejects false mail. It is designed to be very efficient so that it does not slow down the receiving machine.

**greyd** considers sending hosts to be of three types:

* *blacklisted* hosts are redirected to **greyd** and tarpitted i.e. they are communicated with very slowly to consume the sender's resources. Mail is rejected with either a 450 or 550 error message. A blacklisted host will not be allowed to talk to a real mail server.

* *whitelisted* hosts do not talk to **greyd**. Their connections are instead sent to a real mail server.

* *greylisted* hosts are redirected to **greyd**, but **greyd** has not yet decided if they are likely spammers. They are given a temporary failure message by **greyd** when they try to deliver mail.

When **greyd** is run in default mode, it will greylist connections from new hosts. Depending on its configuration, it may choose to blacklist the host or, if the checks described below are met, eventually whitelist it. When **greyd** is run in blacklist-only mode, using the -b flag, it will consult a pre-defined set of blacklist addresses to decide whether to tarpit the host or not.

When a sending host talks to **greyd**, the reply will be stuttered. That is, the response will be sent back a character at a time, slowly. For blacklisted hosts, the entire dialogue is stuttered. For greylisted hosts, the default is to stutter for the first 10 seconds of dialogue only.

The options are as follows (all of which may be specified in **greyd.conf**(5)):

* **-4**:
  For blacklisted entries, return error code 450 to the spammer (default).

* **-5**:
  For blacklisted entries, return error code 550 to the spammer.

* **-6**:
  Enable IPv6. This is disabled by default.

* **-f** *config*:
  The main greyd configuration file.

* **-B** *maxblack*:
  The maximum number of concurrent blacklisted connections to stutter at. This value may not be greater than *maxcon* (see below). The default is maxcon - 100. When this value is exceeded new blacklisted connections will not be stuttered at.

* **-b**:
  Run in blacklist-only mode.

* **-c** *maxcon*:
  The maximum number of concurrent connections to allow. maxcon may not exceed the kernel's maximum number of open files - 200, and defaults to 800.

* **-d**:
  Debug mode in which debug log messages will not be suppressed.

* **-G** *passtime:greyexp:whiteexp*:
  Adjust the three time parameters for greylisting. *passtime* defaults to 25 (minutes), *greyexp* to 4 (hours), and *whiteexp* to 864 (hours, approximately 36 days).

* **-h** *hostname*:
  The hostname that is reported in the SMTP banner.

* **-l** *address*:
  Specify the local address to which **greyd** is to bind. By default **greyd** listens on all local addresses.

* **-L** *address*:
  Specify the local IPv6 address to which **greyd** is to bind. By default **greyd** listens on all local IPv6 addresses. Note, IPv6 must be enabled for this to have any effect.

* **-M** *address*:
  Specify a local IP address which is listed as a low priority MX record, used to identify and trap hosts that connect to MX hosts out of order. See [GREYTRAPPING][] below for details.

* **-n** *name*:
  The SMTP version banner that is reported upon initial connection.

* **-p** *port*:
  Specify a different port number from the default port that **greyd** should listen for redirected SMTP connections on.

* **-S** *secs*:
  Stutter at greylisted connections for the specified amount of seconds, after which the connection is not stuttered at. The default is 10; maximum is 90.

* **-s** *secs*:
  Delay each character sent to the client by the specified amount of seconds. The default is 1; maximum is 10.

* **-v**:
  Enable verbose logging. By default **greyd** logs connections, disconnections and blacklist matches to syslog at *LOG_INFO* level. With verbose logging enabled, message detail including subject and recipient information is logged at *LOG_INFO*, along with the message body and SMTP dialogue being logged at *LOG_DEBUG* level.

* **-w** *window*:
  Set the socket receive buffer to this many bytes, adjusting the window size.

* **-Y** *synctarget*:
  Add target *synctarget* to receive synchronisation messages. synctarget can be either an IPv4 address for unicast messages or a network interface and optional TTL value for multicast messages to the group 224.0.1.241. If the multicast TTL is not specified, a default value of 1 is used. This option can be specified multiple times. If a network interface is specified, it must match the interface specified by the **-y** option. See also [SYNCHRONISATION][] below.

* **-y** *synclisten*:
  Listen on *synclisten* network interface for incoming synchronisation messages. This option can be specified only once. See also [SYNCHRONISATION][] below.

When run in default mode, connections receive the pleasantly innocuous temporary failure of:

    451 Temporary failure, please try again later.

This happens in the SMTP dialogue immediately after the DATA command is received from the client.  **greyd** will use the configured *database driver* to track these connections to **greyd** by connecting IP address, HELO/EHLO, envelope-from, and envelope-to, or tuple for short.

A previously unseen tuple is added to the database, recording the time an initial connection attempt was seen. After *passtime* minutes if **greyd** sees a retried attempt to deliver mail for the same tuple, **greyd** will whitelist the connecting address by adding it as a whitelist entry to the database, and removing the corresponding grey entry.

**greyd** regularly scans the database and configures all whitelist addresses via the configured *firewall driver*, allowing connections to pass to the real MTA. Any addresses not found in the firewall's set management (eg Netfilter IPSets, PF tables, etc.) are redirected to **greyd**.

An example iptables fragment (for use with the *netfilter* driver) is given below. In the example, the *IPSet* *greyd-whitelist* contains the hosts who should be passed directly to the SMTP agent (thus bypassing **greyd**).

    # iptables -t nat -A PREROUTING -p tcp --dport smtp -m set --match-set greyd-whitelist src -j ACCEPT
    # iptables -t nat -A PREROUTING -p tcp --dport smtp -j DNAT --to-destination 127.0.0.1:8025
    # iptables -t filter -A INPUT -p tcp --dport smtp -j ACCEPT
    # iptables -t filter -A INPUT -p tcp --dport 8025 -d 127.0.0.1 -j ACCEPT

Linux kernels by default do not allow routing packets from an external facing network interface to localhost (as used in the above example). To enable this, use the following (update your network interface accordingly):

    # sysctl net.ipv4.conf.eth0.route_localnet=1

**greyd** removes tuple entries from the database if delivery has not been retried within *greyexp* hours from the initial time a connection is seen. The default is 4 hours as this is the most common setting after which MTAs will give up attempting to retry delivery of a message.

**greyd** removes whitelist entries from the database if no mail delivery activity has been seen from the whitelisted address by **greylogd**(8) within *whiteexp* hours from the initial time an address is whitelisted. The default is 36 days to allow for the delivery of monthly mailing list digests without greylist delays every time.

**greyd-setup**(8) should be run periodically by cron to fetch and configure blacklists in **greyd**. When run in blacklist-only mode, the -b flag should be specified. Below is an example crontab entry to run at 5 minutes past every hour:

    05 * * * * /usr/sbin/greyd-setup -f /etc/greyd/greyd.conf

**greylogd**(8) should be used to update the whitelist entries in the configured database when connections are seen to pass to the real MTA on the smtp port.

**greydb**(8) can be used to examine and alter the contents of the configured database. See **greydb**(8) for further information.

**greyd** sends log messages to syslog using facility daemon and, with increasing verbosity, level err, warn, info, and debug. The following rsyslog section can be used to log connection details to a dedicated file:

    if $programname startswith 'grey' then /var/log/greyd.log
    &~

A typical entry shows the time of the connection and the IP address of the connecting host. When a host connects, the total number of active connections and the number of connections from blacklisted hosts is shown (connected (xx/xx)). When a host disconnects, the amount of time spent talking to **greyd** is shown.

## GREYTRAPPING

When running **greyd** in default mode, it may be useful to define spamtrap destination addresses to catch spammers as they send mail from greylisted hosts. Such spamtrap addresses affect only greylisted connections to **greyd** and are used to temporarily blacklist a host that is obviously sending spam. Unused email addresses or email addresses on spammers' lists are very useful for this. When a host that is currently greylisted attempts to send mail to a spamtrap address, it is blacklisted for 24 hours by adding the host to the **greyd** blacklist ⟨*greyd-greytrap*⟩. Spamtrap addresses are added to the database with the following **greydb**(8) command:

    # greydb -T -a 'spamtrap@greyd.org'

See **greydb**(8) for further details.

A file configured with *permitted_domains* in the *grey* section of *greyd.conf* can be used to specify a list of domain name suffixes, one per line, one of which must match each destination email address in the greylist. Any destination address which does not match one of the suffixes listed in *permitted_domains* will be trapped, exactly as if it were sent to a spamtrap address. Comment lines beginning with '#' and empty lines are ignored. A sample *greyd.conf* configuration may be (see **greyd.conf**(5) for further details):

    section grey {
        permitted_domains = "/etc/greyd/permitted_domains",
        ...
    }

For example, if the */etc/greyd/permitted_domains* configured above contains:

* @greyd.org

* obtuse.com

The following destination addresses would not cause the sending host to be trapped:

* beardedclams@greyd.org

* stacy@obtuse.com

* stacy@snouts.obtuse.com

However the following addresses would cause the sending host to be trapped:

* peter@bugs.greyd.org

* bigbutts@bofh.ucs.ualberta.ca

A low priority MX IP address may be specified with the -M option. When **greyd** has such an address specified, no host may enter new greylist tuples when connecting to this address; only existing entries may be updated. Any host attempting to make new deliveries to the low priority MX for which a tuple has not previously been seen will be trapped.

Note that it is important to ensure that a host running **greyd** with the low priority MX address active must see all the greylist changes for a higher priority MX host for the same domains. This is best done by the host itself receiving the connections to the higher priority MX on another IP address (which may be an IP alias). This will ensure that hosts are not trapped erroneously if the higher priority MX is unavailable. For example, on a host which is an existing MX record for a domain of value 10, a second IP address with MX of value 99 (a higher number, and therefore lower priority) would ensure that any RFC conformant client would attempt delivery to the IP address with the MX value of 10 first, and should not attempt to deliver to the address with MX value 99.

## BLACKLIST-ONLY MODE

When running in default mode, the *iptables* rules described above are sufficient (when using the *netfilter* firewall driver). However when running in blacklist-only mode, a slightly modified iptables ruleset is required, redirecting any addresses found in the ⟨*greyd-blacklist*⟩ IPSet to **greyd**. Any other addresses are passed to the real MTA. For example:

    # iptables -t nat -A PREROUTING -p tcp --dport smtp \
        -m set --match-set greyd-blacklist src -j DNAT --to-destination 127.0.0.1:8025
    # iptables -t filter -A INPUT -p tcp --dport smtp -j ACCEPT
    # iptables -t filter -A INPUT -p tcp --dport 8025 -d 127.0.0.1 -j ACCEPT

Addresses can be loaded into the table with the *ipset* command (consult the *ipset* manual for more details), like:

    # ipset add greyd-blacklist 1.2.3.4/30

**greyd-setup**(8) can also be used to load addresses into the ⟨*greyd-blacklist*⟩ table. It has the added benefit of being able to remove addresses from blacklists, and will connect to **greyd** over a localhost socket, giving **greyd** information about each source of blacklist addresses, as well as custom rejection messages for each blacklist source that can be used to let any real person whose mail is deferred by **greyd** know why their address has been listed from sending mail. This is important as it allows legitimate mail senders to pressure spam sources into behaving properly so that they may be removed from the relevant blacklists.

## CONFIGURATION CONNECTIONS

**greyd** listens for configuration connections on port 8026 by default, which can be overridden by setting the *config_port* configuration option. The configuration socket listens only on the INADDR_LOOPBACK address. Configuration of **greyd** is done by connecting to the configuration socket, and sending blacklist information. Each blacklist consists of a name, a message to reject mail with, and addresses in CIDR format. This information is specified in the **greyd.conf** format, with entries terminated by '%%'. For example:

    name = "greyd-blacklist
    message = "Your IP address %A has been blocked by \\\\nour blacklist"
    ips = [ "1.3.4.2/31", "2.3.4.5/30", "1.2.3.4/32" ]
    %%

A \" will produce a double quote in the output. \\\\n will produce a newline. %A will expand to the connecting IP address in dotted quad format. %% may be used to produce a single % in the output. \\ will produce a single \. **greyd** will reject mail by displaying all the messages from all blacklists in which a connecting address is matched. **greyd-setup**(8) is normally used to configure this information.

## SYNCHRONISATION

**greyd** supports realtime synchronisation of **greyd** databases between a number of **greyd** daemons running on multiple machines, using the **-Y** and **-y** options. The databases are synchronised for greylisted, trapped and whitelisted entries. Entries made manually using **greydb**(8) are also synchronised (if using the same *sync* section configuration in *greyd.conf*(5)). Furthermore, hosts whitelisted by **greylogd**(8) are also synchronised with the appropriate configuration in the same manner as **greydb**(8).

The following example will accept incoming multicast and unicast synchronisation messages, and send outgoing multicast messages through the network interface eth0:

    # greyd -y eth0 -Y eth0

The second example will increase the multicast TTL to a value of 2, add the unicast targets foo.somewhere.org and bar.somewhere.org, and accept incoming unicast messages received on eth0 only.

    # greyd -y eth0:2 -Y eth0:2 -Y foo.somewhere.org -Y bar.somewhere.org

If a *key* file is specified in the *sync* **greyd.conf**(5) configuration section and exists, **greyd** will calculate the message-digest fingerprint (checksum) for the file and use it as a shared key to authenticate the synchronisation messages. Below is an example sync configuration (see **greyd.conf**(5) for more details):

    section sync {
        verify = 1,
        key = "/etc/greyd/greyd.key",
        ...
    }

The file itself can contain any data. For example, to create a secure random key:

    # dd if=/dev/random of=/etc/greyd/greyd.key bs=2048 count=1

The file needs to be copied to all hosts sending or receiving synchronisation messages.

## SEE ALSO

  greyd.conf(5), greyd-setup(8), greydb(8), greylogd(8)

## HISTORY

**greyd** closly follows the design of the *OpenBSD* *spamd*, and thus implements all features of *spamd*. Essentially all of the code is written from scratch, with other notable differences from *spamd*:

* The code is modular to support good test coverage by way of unit testing.

* The system abstracts the interfaces to the firewall and database, to support a wide variety of setups (eg GNU/Linux).

* The system is designed to make use of common configuration file(s) between **greyd**, **greylogd**, **greydb** & **greyd-setup**.

## CREDITS

Much of this man page was taken from the *OpenBSD* manual, and adapted accordingly.
