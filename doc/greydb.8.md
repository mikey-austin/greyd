greydb(8) -- greyd database tool
================================

## SYNOPSIS

`greydb` [**-f** config] [[**-Tt**] **-a** keys] [[**-Tt**] **-d** keys]

## DESCRIPTION

**greydb** manipulates the **greyd** database used for **greyd**(8).

The options are as follows:

* **-f** *config*:
The main greyd configuration file.

* **-a** *keys*:
Add or update the entries for keys. This can be used to whitelist one or more IP addresses (i.e. circumvent the greylisting process altogether) by adding all IP addresses as keys to the **greyd** database for WHITE entries. If any keys specified match entries already in the **greyd** database, **greydb** updates the entry's time last seen to now.

* **-d** *keys*:
Delete entries for keys.

* **-T**:
Add or delete the keys as SPAMTRAP entries. See the GREYTRAPPING section of **greyd**(8) for more information. Must be used in conjunction with the **-a** or **-d** option.

* **-t**:
Add or delete the keys as TRAPPED entries. See the GREYTRAPPING section of **greyd**(8) for more information. Must be used in conjunction with the **-a** or **-d** option.

If adding or deleting a SPAMTRAP address (**-T**), keys should be specified as email addresses:

    spamtrap@mydomain.org

Otherwise keys must be numerical IP addresses.

## DATABASE OUTPUT FORMAT

If invoked without any arguments, **greydb** lists the contents of the database in a text format.

For SPAMTRAP entries the format is:

    type|mailaddress

where type will be SPAMTRAP and mailaddress will be the email address for which any connections received by **greyd**(8) will be blacklisted if mail is sent to this address.

For TRAPPED entries the format is:

    type|ip|expire

where *type* will be TRAPPED, IP will be the IP address blacklisted due to hitting a spamtrap, and *expire* will be when the IP is due to be removed from the blacklist.

For GREY entries, the format is:

    type|source IP|helo|from|to|first|pass|expire|block|pass

For WHITE entries, the format is:

    type|source IP|||first|pass|expire|block|pass

The fields are as follows:

* **type**:
  WHITE if whitelisted or GREY if greylisted

* **source IP**:
  IP address the connection originated from

* **helo**:
  what the connecting host sent as identification in the HELO/EHLO command in the SMTP dialogue

* **from**:
  envelope-from address for GREY (empty for WHITE entries)

* **to**:
  envelope-to address for GREY (empty for WHITE entries)

* **first**:
  time the entry was first seen

* **pass**:
  time the entry passed from being GREY to being WHITE

* **expire**:
  time the entry will expire and be removed from the database

* **block**:
  number of times a corresponding connection received a temporary failure from **greyd**(8)

* **pass**:
  number of times a corresponding connection has been seen to pass to the real MTA by **greylogd**(8)

Note that times are in seconds since the Epoch, in the manner returned by time(3). Times may be converted to human readable format using:

    $ date --date '@<value>'

## SEE ALSO

  **greyd.conf**(5), **greyd-setup**(8), **greydb**(8), **greylogd**(8)

## CREDITS

Much of this man page was taken from the *OpenBSD* manual, and adapted accordingly.
