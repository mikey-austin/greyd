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
                    <a href="https://github.com/mikey-austin/greyd/archive/v0.9.2.tar.gz">greyd (v0.9.2)</a> - 2015-06-08
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
            <h3>Installation</h3>

            <p>
                To build on GNU/Linux with the <em>SQLite DB driver</em> and the <em>Netfilter driver</em>, the following configure flags will do the trick (see ./configure --help for more):
            </p>

            <div class="highlight">
<code>$ tar xzf greyd-0.9.0.tar.gz && cd greyd-0.9.0
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
