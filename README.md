greyd - A mail greylisting & blacklisting daemon
================================================

Overview
--------

**greyd** is derived from OpenBSD's **spamd** spam deferral daemon and supporting programs.
As **spamd** is tightly integrated with the PF firewall, there are no production-ready
ports available for the GNU/Linux world. In addition to providing equivalent features
to **spamd**, **greyd** aims to:

  - Be firewall agnostic, and provide a generic interface for pluggable modules,
    to allow operation with *any* suitable firewall (eg iptables/ipset/netfilter, FreeBSD's
    IPFW, etc.).

  - Provide a generic database interface for pluggable modules to work with a variety
    of different databases (eg Berkeley DB, SQLite, MySQL, Postgres, etc.).

  - Be portable and run on many different systems (eg GNU/Linux, the BSD's, etc.)

  - Have all of the programs in the **greyd** suite be driven by flexible configuration files,
    in addition to supporting the same command line switches as **spamd** & friends.

  - Have a clean & modularized internal structure, to facilitate unit & regression testing
    (there are currently > 480 tests).

  - Be able to import the same blacklists & whitelists that **spamd** can import.

The greyd suite
-----------------

**greyd** provides analogous versions of each of the **spamd** programs, namely:

  * **greyd**       - the main spam deferral daemon
  * **greydb**      - greylisting/greytrapping database management
  * **greyd-setup** - blacklist & whitelist population
  * **greylogd**    - connection tracking & whitelist updating

Development Status
------------------

**greyd** is currently under active development and is now fully functional. All of the features from **spamd**
have now been implemented, including synchronization support.

A database driver has been implemented for Berkeley DB (4.x onwards), which makes full use of transactions (in contrast to the OpenBSD version). After the first
release, a MySQL driver will be implemented.

A firewall driver has been implemented for the netfilter ecosystem. This driver makes use of:
  * **libipset** for IP set management
  * **libnetfilter-log** for the tracking and auto-whitelisting of connections
  * **libnetfilter-conntrack** for the DNAT original destination lookups

Firewall drivers for PF & IPFW are planned as well, which will allow **greyd** to run in a BSD environment.

Before the first proper release, there is still the following to be done:
  * Testing in the wild on different setups
  * <del>autotools build configuration</del>
  * <del>man pages & documentation (install guides, user guides, etc.)</del>
  * sample init scripts for some RHEL-like systems (and Debian)

Licensing
---------

All of the source is (will be) licensed under the OpenBSD license, with the exception of the netfilter
firewall driver. As this driver links with the libnetfilter userland libraries, it must be licensed
under the GPL. This does not conflict with the rest of the code base as all **greyd** drivers are
compiled as shared objects, to be dynamically linked at runtime.
