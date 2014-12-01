greyd - A mail greylisting & blacklisting daemon
================================================

Overview
--------

`greyd` is derived from OpenBSD's `spamd` spam deferral daemon and supporting programs.
As `spamd` is tightly integrated with the PF firewall, there are no production-ready
ports available for the GNU/Linux world. In addition to providing equivalent features
to `spamd`, `greyd` aims to:

  - Be firewall agnostic, and provide a generic interface for pluggable modules,
    to allow operation with *any* suitable firewall (eg iptables/ipset/netfilter, FreeBSD's
    IPFW, etc.).

  - Provide a generic database interface for pluggable modules to work with a variety
    of different databases (eg Berkeley DB, SQLite, MySQL, Postgres, etc.).

  - Be portable and run on many different systems (eg GNU/Linux, the BSD's, etc.)

  - Have all of the programs in the `greyd` suite be driven by flexible configuration files,
    in addition to supporting the same command line switches as `spamd` & friends.

  - Have a clean & modularized internal structure, to facilitate unit & regression testing
    (there are currently > 440 tests).

  - Be able to import the same blacklists & whitelists that `spamd` can import.

The greyd suite
-----------------

`greyd` provides analogous versions of each of the `spamd` programs, namely:

  - `greyd`       - the main spam deferral daemon
  - `greydb`      - greylisting/greytrapping database management
  - `greyd-setup` - blacklist & whitelist population
  - `greylogd`    - connection tracking & whitelist updating

Development Status
------------------

`greyd` is currently under active development and is around 85% complete. The `greyd-setup` and `greydb`
programs are complete, with the main `greyd` and `greylogd` on the way.

I also aim to autotool-ize the project after the main components are complete. All development
is currently happening on Fedora.
