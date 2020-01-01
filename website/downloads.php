<?php $active = 'downloads'; include('header.php'); ?>

<section class="banner">
    <div class="content">
        <h2>
            Downloads
        </h2>
    </div>
</section>

<section>
    <div class="content group">
        <div class="tile wide">
            <h3>Download a source tarball</h3>

            <p>Grab the latest distribution:</p>
            <ul>
                <li>
                    <a href="https://github.com/mikey-austin/greyd/releases/download/v0.11.2/greyd-0.11.2.tar.gz">greyd-0.11.2.tar.gz</a> - 2019-12-30
                </li>
            </ul>
            <p>
                You will need the following libraries (and their headers) installed:
            </p>
            <ul>
                <li>libltdl (Libtool's libdl abstraction)</li>
                <li>zlib</li>
                <li>OpenSSL (libcrypto)</li>
                <li>Berkeley DB (either 4.x or & 5.x), SQLite 3, Berkeley DB SQL or MySQL</li>
            </ul>
            <p>
                To build the netfilter driver, you will need the following packages:
            </p>
            <ul>
                <li>libnetfilter_conntrack (version >= 1.0.4)</li>
                <li>libnetfilter_log</li>
                <li>libipset</li>
                <li>libcap</li>
                <li>libmnl</li>
            </ul>
        </div>

        <div class="tile wide">
          <h3>Installation via greyd.org repo (Centos 7)</h3>

          <p>
            To use the repository you can add the <strong>greyd.repo</strong> file like so:
          </p>
            <div class="highlight">
<code>$ cat /etc/yum.repos.d/greyd.repo 
[greyd]
name=greyd Packages for Enterprise Linux 7 - $basearch
baseurl=https://greyd.org/repo/centos/7/$basearch
enabled=1
gpgcheck=1
gpgkey=https://greyd.org/repo/greyd_pkg_sign_pub.asc</code>
            </div>
          <p>
            Then be sure to accept the greyd GPG signing key:
          </p>

            <div class="highlight">
<code>Retrieving key from https://greyd.org/repo/greyd_pkg_sign_pub.asc
Importing GPG key 0x5425A1D0:
 Userid     : "Mikey Austin <mikey@greyd.org>"
 Fingerprint: c3c7 ddb8 db4c 4f5b a4eb 7e67 6686 de7c 5425 a1d0
 From       : https://greyd.org/repo/greyd_pkg_sign_pub.asc
Is this ok [y/N]: y</code></div>

            <h3>Installation from Source</h3>

            <p>
                To build on GNU/Linux with the <em>SQLite DB driver</em> and the <em>Netfilter driver</em>, the following configure flags will do the trick (see ./configure --help for more):
            </p>

            <div class="highlight">
<code>$ tar xzf greyd-0.11.2.tar.gz && cd greyd-0.11.2
$ ./configure --with-sqlite --with-netfilter
$ make
$ sudo make install</code>
            </div>

            <p>
                There is an issue on some distros (eg Arch Linux) with the system ltdl.h and a missing <strong>lt__PROGRAM__LTX_preloaded_symbols</strong> symbol. The <strong>--with-ltdl-fix</strong> flag will enable a workaround for such situations.
            </p>

            <p>
                On an RPM-based distribution, you can easily make a package by the following:
            </p>

            <div class="highlight">
<code>$ make rpm</code>
            </div>
        </div>
    </div>
</section>

 <?php include('footer.php') ?>
