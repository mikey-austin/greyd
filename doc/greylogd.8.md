greylogd(8) -- greyd whitelist updating daemon
==============================================

## SYNOPSIS

`greylogd` [**-dI**] [**-f** config] [**-p** port] [**-P** pidfile] [**-W** whiteexp] [**-Y** synctarget]

## DESCRIPTION

**greylogd** manipulates the **greyd**(8) database used for greylisting. **greylogd** updates the database whitelist entries whenever a connection to port 25 is logged by the configured firewall; see [CONNECTION TRACKING][] below. The source addresses of inbound connections are whitelisted when seen by **greylogd** to ensure that their entries in the database do not expire if the connecting host continues to send legitimate mail. The destination addresses of outbound connections are whitelisted when seen by **greylogd** so that replies to outbound mail may be received without initial greylisting delays. Greylisting is explained more fully in **greyd**(8).

The options are as follows:

* **-f** *config*:
  The main greyd configuration file.

* **-d**:
  Debugging mode. **greylogd** displays debug messages (suppressed by default).

* **-P** *pidfile*:
Specify the location for the pidfile.

* **-I**:
  Specify that **greylogd** is only to whitelist inbound SMTP connections. By default **greylogd** will whitelist the source of inbound SMTP connections, and the target of outbound SMTP connections.

* **-W** *whiteexp*:
  Adjust the time for *whiteexp* in hours. The default is 864 hours (approximately 36 days); maximum is 2160 hours (approximately 90 days).

* **-Y** *synctarget*:
  Add a target to receive synchronisation messages; see [SYNCHRONISATION][] below. This option can be specified multiple times.

It is important to log any connections to and from the real MTA in order for **greylogd** to update the whitelist entries. See **greyd**(8) for an example ruleset for logging such connections.

**greylogd** sends log messages to syslogd(8) using facility daemon. **greylogd** will log each connection it sees at level LOG_DEBUG.

## CONNECTION TRACKING

The tracking of connections is firewall dependent. When using the *netfilter* firewall driver, the iptables *NFLOG* target must be used to specify the inbound/outbound connections of interest via the appropriate *--nflog-group*. For example, to log the inbound connections the following iptables rules may be used (155 is the default inbound *--nflog-group*):

    # iptables -t nat -A PREROUTING -p tcp --dport smtp \
        -m set --match-set greyd-whitelist src -j NFLOG --nflog-group 155

and similarly for tracking outbound connections (255 is the default outbound *--nflog-group*):

    # iptables -t filter -A OUTPUT -m conntrack --ctstate NEW \
        -p tcp --dport 25 -j NFLOG --nflog-group 255

For the *netfilter* driver, the above default configuration may be overridden in **greyd.conf**(5), for example:

    section firewall {
        driver = "netfilter.so" # Find via dynamic linker
        track_outbound = 1
        inbound_group = 155
        outbound_group = 255
        ...
    }

For the *pf* firewall driver, the following PF rules will log the packets appropriately:

    table <greyd-whitelist> persist
    pass in on egress proto tcp from any to any port smtp rdr-to 127.0.0.1 port 8025
    pass in log on egress proto tcp from <greyd-whitelist> to any port smtp
    pass out log on egress proto tcp to any port smtp

See **greyd.conf**(5) for more details.

## SYNCHRONISATION

**greylogd** supports realtime synchronisation of whitelist states by sending the information it updates to a number of **greyd**(8) daemons running on multiple machines. To enable synchronisation, use the command line option -Y to specify the machines to which **greylogd** will send messages when it updates the state information. The synchronisation may also be configured entirely via **greyd.conf**(5). For more information, see **greyd**(8) and **greyd.conf**(5).

## COPYRIGHT

**greylogd** is Copyright (C) 2015 Mikey Austin (greyd.org)

## SEE ALSO

  **greyd.conf**(5), **greyd-setup**(8), **greydb**(8), **greyd**(8)

## CREDITS

Much of this man page was taken from the *OpenBSD* manual, and adapted accordingly.
