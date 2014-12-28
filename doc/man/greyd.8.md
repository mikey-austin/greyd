greyd(8) -- spam deferral daemon
=================================

## SYNOPSIS

`greyd` [**-456bdv**] [**-f** config] [**-B** maxblack] [**-c** maxcon] [**-G** passtime:greyexp:whiteexp] [**-h** hostname] [**-l** address] [**-L** address] [**-M** address] [**-n** name] [**-p** port] [**-S** secs] [**-s** secs] [**-w** window] [**-Y** synctarget] [**-y** synclisten]

## DESCRIPTION

**greyd** is a fake mail daemon which rejects false mail. It is designed to be very efficient so that it does not slow down the receiving machine.

**greyd** considers sending hosts to be of three types:

*blacklisted* hosts are redirected to **greyd** and tarpitted i.e. they are communicated with very slowly to consume the sender's resources. Mail is rejected with either a 450 or 550 error message. A blacklisted host will not be allowed to talk to a real mail server.

*whitelisted* hosts do not talk to **greyd**. Their connections are instead sent to a real mail server, such as smtpd(8).

*greylisted* hosts are redirected to **greyd**, but **greyd** has not yet decided if they are likely spammers. They are given a temporary failure message by **greyd** when they try to deliver mail.

When **greyd** is run in default mode, it will greylist connections from new hosts. Depending on its configuration, it may choose to blacklist the host or, if the checks described below are met, eventually whitelist it. When **greyd** is run in blacklist-only mode, using the -b flag, it will consult a pre-defined set of blacklist addresses to decide whether to tarpit the host or not.

When a sending host talks to **greyd**, the reply will be stuttered. That is, the response will be sent back a character at a time, slowly. For blacklisted hosts, the entire dialogue is stuttered. For greylisted hosts, the default is to stutter for the first 10 seconds of dialogue only.

The options are as follows:

* **-4**:
  For blacklisted entries, return error code 450 to the spammer (default).

* **-5**:
  For blacklisted entries, return error code 550 to the spammer.

* **-6**:
  Enable IPv6. This is disabled by default.

* **-f** *config*:
  The main greyd configuration file.

* **-B** *maxblack*:
  The maximum number of concurrent blacklisted connections to stutter at. This value may not be greater than maxcon (see below). The default is maxcon - 100. When this value is exceeded new blacklisted connections will not be stuttered at.

* **-b**:
  Run in blacklist-only mode.

* **-c** *maxcon*:
  The maximum number of concurrent connections to allow. maxcon may not exceed kern.maxfiles - 200, and defaults to 800.

* **-d**:
  Debug mode. **greyd** does not fork(2) into the background.

* **-G** *passtime:greyexp:whiteexp*:
  Adjust the three time parameters for greylisting. passtime defaults to 25 (minutes), greyexp to 4 (hours), and whiteexp to 864 (hours, approximately 36 days).

* **-h** *hostname*:
  The hostname that is reported in the SMTP banner.

* **-l** *address*:
  Specify the local address to which **greyd** is to bind(2). By default **greyd** listens on all local addresses.

* **-L** *address*:
  Specify the local IPv6 address to which **greyd** is to bind(2). By default **greyd** listens on all local IPv6 addresses. Note, IPv6 must be enabled for this to have any effect.

* **-M** *address*:
  Specify a local IP address which is listed as a low priority MX record, used to identify and trap hosts that connect to MX hosts out of order. See GREYTRAPPING below for details.

* **-n** *name*:
  The SMTP version banner that is reported upon initial connection.

* **-p** *port*:
  Specify a different port number from the default port that **greyd** should listen for redirected SMTP connections on.

* **-S** *secs*:
  Stutter at greylisted connections for the specified amount of seconds, after which the connection is not stuttered at. The default is 10; maximum is 90.

* **-s** *secs*:
  Delay each character sent to the client by the specified amount of seconds. The default is 1; maximum is 10.

* **-v**:
  Enable verbose logging. By default **greyd** logs connections, disconnections and blacklist matches to syslogd(8) at LOG_INFO level. With verbose logging enabled, message detail including subject and recipient information is logged at LOG_INFO, along with the message body and SMTP dialogue being logged at LOG_DEBUG level.

* **-w** *window*:
  Set the socket receive buffer to this many bytes, adjusting the window size.

* **-Y** *synctarget*:
  Add target synctarget to receive synchronisation messages. synctarget can be either an IPv4 address for unicast messages or a network interface and optional TTL value for multicast messages to the group 224.0.1.240. If the multicast TTL is not specified, a default value of 1 is used. This option can be specified multiple times. See also SYNCHRONISATION below.

* **-y** *synclisten*:
  Listen on synclisten network interface for incoming synchronisation messages. This option can be specified only once. See also SYNCHRONISATION below.

When run in default mode, connections receive the pleasantly innocuous temporary failure of:

  451 Temporary failure, please try again later.

This happens in the SMTP dialogue immediately after the DATA command is received from the client.  **greyd** will use the db file in /var/db/spamd to track these connections to **greyd** by connecting IP address, HELO/EHLO, envelope-from, and envelope-to, or tuple for short.

A previously unseen tuple is added to the /var/db/spamd database, recording the time an initial connection attempt was seen. After passtime minutes if **greyd** sees a retried attempt to deliver mail for the same tuple, **greyd** will whitelist the connecting address by adding it as a whitelist entry to /var/db/spamd.

**greyd** regularly scans the /var/db/spamd database and configures all whitelist addresses as the pf(4) ⟨greyd-whitelist⟩ table, allowing connections to pass to the real MTA. Any addresses not found in ⟨greyd-whitelist⟩ are redirected to **greyd**.

An example pf.conf(5) fragment is given below. In the example, the file /etc/mail/nospamd contains addresses of hosts who should be passed directly to the SMTP agent (thus bypassing **greyd**).

**greyd** removes tuple entries from the /var/db/spamd database if delivery has not been retried within greyexp hours from the initial time a connection is seen. The default is 4 hours as this is the most common setting after which MTAs will give up attempting to retry delivery of a message.

**greyd** removes whitelist entries from the /var/db/spamd database if no mail delivery activity has been seen from the whitelisted address by spamlogd(8) within whiteexp hours from the initial time an address is whitelisted. The default is 36 days to allow for the delivery of monthly mailing list digests without greylist delays every time.

**greyd-setup**(8) should be run periodically by cron(8). When run in blacklist-only mode, the -b flag should be specified. Use crontab(1) to uncomment the entry in root's crontab.

spamlogd(8) should be used to update the whitelist entries in /var/db/spamd when connections are seen to pass to the real MTA on the smtp port.

**greydb**(8) can be used to examine and alter the contents of /var/db/spamd. See **greydb**(8) for further information.

**greyd** sends log messages to syslogd(8) using facility daemon and, with increasing verbosity, level err, warn, info, and debug. The following syslog.conf(5) section can be used to log connection details to a dedicated file:

A typical entry shows the time of the connection and the IP address of the connecting host. When a host connects, the total number of active connections and the number of connections from blacklisted hosts is shown (connected (xx/xx)). When a host disconnects, the amount of time spent talking to **greyd** is shown.

## GREYTRAPPING

When running **greyd** in default mode, it may be useful to define spamtrap destination addresses to catch spammers as they send mail from greylisted hosts. Such spamtrap addresses affect only greylisted connections to **greyd** and are used to temporarily blacklist a host that is obviously sending spam. Unused email addresses or email addresses on spammers' lists are very useful for this. When a host that is currently greylisted attempts to send mail to a spamtrap address, it is blacklisted for 24 hours by adding the host to the **greyd** blacklist ⟨greyd-greytrap⟩. Spamtrap addresses are added to the /var/db/spamd database with the following **greydb**(8) command:

    # greydb -T -a 'spamtrap@greyd.org'

See **greydb**(8) for further details.

The file /etc/mail/spamd.alloweddomains can be used to specify a list of domainname suffixes, one per line, one of which must match each destination email address in the greylist. Any destination address which does not match one of the suffixes listed in spamd.alloweddomains will be trapped, exactly as if it were sent to a spamtrap address. Comment lines beginning with ‘#’ and empty lines are ignored.

For example, if spamd.alloweddomains contains:

  * @humpingforjesus.com
  * obtuse.com

The following destination addresses would not cause the sending host to be trapped:

  * beardedclams@humpingforjesus.com
  * beck@obtuse.com
  * beck@snouts.obtuse.com

However the following addresses would cause the sending host to be trapped:

  * peter@apostles.humpingforjesus.com
  * bigbutts@bofh.ucs.ualberta.ca

A low priority MX IP address may be specified with the -M option. When **greyd** has such an address specified, no host may enter new greylist tuples when connecting to this address; only existing entries may be updated. Any host attempting to make new deliveries to the low priority MX for which a tuple has not previously been seen will be trapped.

Note that it is important to ensure that a host running **greyd** with the low priority MX address active must see all the greylist changes for a higher priority MX host for the same domains. This is best done by the host itself receiving the connections to the higher priority MX on another IP address (which may be an IP alias). This will ensure that hosts are not trapped erroneously if the higher priority MX is unavailable. For example, on a host which is an existing MX record for a domain of value 10, a second IP address with MX of value 99 (a higher number, and therefore lower priority) would ensure that any RFC conformant client would attempt delivery to the IP address with the MX value of 10 first, and should not attempt to deliver to the address with MX value 99.

## BLACKLIST-ONLY MODE

When running in default mode, the pf.conf(5) rules described above are sufficient. However when running in blacklist-only mode, a slightly modified pf.conf(5) ruleset is required, redirecting any addresses found in the ⟨**greyd**⟩ table to **greyd**. Any other addresses are passed to the real MTA.


Addresses can be loaded into the table, like:

**greyd-setup**(8) can also be used to load addresses into the ⟨**greyd**⟩ table. It has the added benefit of being able to remove addresses from blacklists, and will connect to **greyd** over a localhost socket, giving **greyd** information about each source of blacklist addresses, as well as custom rejection messages for each blacklist source that can be used to let any real person whose mail is deferred by **greyd** know why their address has been listed from sending mail. This is important as it allows legitimate mail senders to pressure spam sources into behaving properly so that they may be removed from the relevant blacklists.

## CONFIGURATION CONNECTIONS

**greyd** listens for configuration connections on port 8026 by default, which can be overridden by setting the *config_port* configuration option. The configuration socket listens only on the INADDR_LOOPBACK address. Configuration of **greyd** is done by connecting to the configuration socket, and sending blacklist information, one blacklist per line. Each blacklist consists of a name, a message to reject mail with, and addresses in CIDR format, all separated by semicolons (;):

The rejection message must be inside double quotes. A \" will produce a double quote in the output. \n will produce a newline. %A will expand to the connecting IP address in dotted quad format. %% may be used to produce a single % in the output. \\ will produce a single \. **greyd** will reject mail by displaying all the messages from all blacklists in which a connecting address is matched. **greyd-setup**(8) is normally used to configure this information.

## SYNCHRONISATION

**greyd** supports realtime synchronisation of **greyd** databases between a number of **greyd** daemons running on multiple machines, using the -Y and -y options. The databases are synchronised for greylisted and trapped entries; whitelisted entries and entries made manually using **greydb**(8) are not updated.

The following example will accept incoming multicast and unicast synchronisation messages, and send outgoing multicast messages through the network interface em0:

    # greyd -y eth0 -T eth0

The second example will increase the multicast TTL to a value of 2, add the unicast targets foo.somewhere.org and bar.somewhere.org, and accept incoming unicast messages received on bge0 only.

    # greyd -y bge0 -Y em0:2 -Y foo.somewhere.org -Y bar.somewhere.org

If the file /etc/mail/spamd.key exists, **greyd** will calculate the message-digest fingerprint (checksum) for the file and use it as a shared key to authenticate the synchronisation messages. The file itself can contain any data. For example, to create a secure random key:

    # dd if=/dev/random of=/etc/mail/spamd.key bs=2048 count=1

The file needs to be copied to all hosts sending or receiving synchronisation messages.

## SEE ALSO

  greyd.conf(5), greyd-setup(8), greydb(8), greylogd(8)

## CREDITS

Much of this man page was taken from the *OpenBSD* manual.