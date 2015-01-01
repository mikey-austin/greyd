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
                    <a href="downloads/greyd-0.1.0.tar.gz">greyd-0.1.0.tar.gz</a> - 2015-01-01
                </li>
            </ul>
            <p>
                You will need the following libraries (and their headers) installed:
            </p>
            <ul>
                <li>libltdl (Libtool's libdl abstraction)</li>
                <li>zlib</li>
                <li>OpenSSL (libcrypto)</li>
                <li>Berkeley DB (tested with >= 4.7 & 5.3)</li>
                <li>libnetfilter_conntrack</li>
                <li>libnetfilter_log</li>
                <li>libipset</li>
            </ul>
        </div>

        <div class="tile wide">
            <h3>Installation</h3>

            <p>
                To build on GNU/Linux with the <em>Berkeley DB driver</em> and the <em>Netfilter driver</em>, the following configure flags will do the trick (see ./configure --help for more):
            </p>

            <div class="highlight">
<code>$ tar xzf greyd-0.1.0.tar.gz && cd greyd-0.1.0
$ ./configure --with-bdb --with-netfilter
$ make
$ sudo make install</code>
            </div>

            <p>
                Note that by default, everything will be installed into <em>/usr/local</em>. The following configure flags may be used to override these locations:
            </p>

            <ul>
                <li><strong>--prefix</strong> (eg /usr)</li>
                <li><strong>--sysconfdir</strong> (eg /etc)</li>
                <li><strong>--localstatedir</strong> (eg /var)</li>
            </ul>
        </div>
    </div>
</section>

 <?php include('footer.php') ?>
