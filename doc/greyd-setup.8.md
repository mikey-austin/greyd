greyd-setup(8) -- parse and load file of spammer addresses
==========================================================

## SYNOPSIS

`greyd-setup` [**-bDdn**] [**-f** config]

## DESCRIPTION

The **greyd-setup** utility sends blacklist data to **greyd**(8), as well as configuring mail rejection messages for blacklist entries.

When **greyd-setup** is run in blacklist only mode, it also sends blacklist data to the firewall *greyd-blacklist* table (or IPSet if using *netfilter* firewall driver). The ⟨*greyd-blacklist*⟩ table must then be used in conjunction with a firewall redirection rule to selectively redirect mail connections to **greyd**(8).

The options are as follows:

* **-f** *config*:
  The main greyd configuration file.

* **-b**:
  Blacklisting only mode. Blacklist data is normally stored only in **greyd**(8). With this flag, data is stored in both **greyd**(8) and the firewall. Use this flag if **greyd**(8) is running with the -b flag too.

* **-D**:
  Daemonize; run **greyd-setup** in the background.

* **-d**:
  Debug mode reports a few pieces of information.

* **-n**:
  Dry-run mode. No data is shipped.

Lists are specified in the configuration file **greyd.conf**(5) and are processed in the order specified in the *lists* tag in the *setup* section, for example:

    section setup {
        lists = [ "nixspam", "uatraps" ],
        curl_path = "/usr/bin/curl"
    }

with the following list definitions:

    blacklist nixspam {
        message = "Your address %A is in the nixspam list",
        method  = "http",
        file = "www.openbsd.org/spamd/nixspam.gz"
    }

    blacklist nixspam {
        message = "Your address %A has sent mail to a ualberta.ca spamtrap\\\\n
                   within the last 24 hours",
        method = "http",
        file = "www.openbsd.org/spamd/traplist.gz"
    }

The *http* method specified in the above blacklist definitions will instruct **greyd-setup** to fetch the lists using *curl*.

Output is concatenated and sent to a running **greyd**(8). Addresses are sent along with the message **greyd** will give on mail rejection when a matching client connects. The configuration port for **greyd**(8) is found from the *config_port* configuration option in **greyd.conf**(5) (which defaults to port 8026).

**greyd-setup** reads all configuration information from the spamd.conf(5) file.

## SEE ALSO

  **greyd.conf**(5), **greyd**(8), **greydb**(8), **greylogd**(8)

## CREDITS

Much of this man page was taken from the *OpenBSD* manual, and adapted accordingly.
