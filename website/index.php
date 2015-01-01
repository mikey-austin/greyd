<?php include('header.php') ?>

<section class="banner home">
    <div class="content">
        <h2>
            Block spam before it steps foot in your network.
        </h2>
    </div>
</section>

<section>
    <div class="content group">
        <div class="tile">
            <h3>What is it?</h3>
            <p>
                <em>greyd</em> is a lightweight greylisting &amp; blacklisting daemon. It runs on your gateway and pretends to be a real mail server. It keeps track of all hosts who connect, and automatically whitelists hosts after a period.
            </p>
            <p>
                <em>greyd</em> is written in <strong>C</strong>, and endeavours to be portable (ie <strong>POSIX</strong> compliant).
            </p>
            <p>
                <em>greyd</em> consists of four programs:
            </p>
            <ul>
                <li><strong>greyd</strong> - the main spam deferral daemon.</li>
                <li><strong>greylogd</strong> - connection tracking & whitelist updating.</li>
                <li><strong>greydb</strong> - greylisting/greytrapping database management.</li>
                <li><strong>greyd-setup</strong> - blacklist & whitelist population.</li>
            </ul>
        </div>

        <div class="tile">
            <h3>Which systems are supported?</h3>
            <p>
                A <strong>netfilter</strong> firewall driver has been implemented for <em>greyd</em> to run on <strong>GNU/Linux</strong> systems.
            </p>
            <p>
                <em>greyd</em> closely integrates with the firewall, and provides an abstract interface to theoretically support any appropriate firewall system.
            </p>
            <p>
                One of the goals of the <em>greyd</em> project is to support a wide variety of systems, firewalls and databases.
            </p>
        </div>

        <div class="tile">
            <h3>Blacklisting</h3>
            <p>
                Blacklists may also be loaded into <em>greyd</em>. If <em>greyd</em> finds that a host is on one or more blacklists, it will reply <em>very slowly</em> in order to waste as much spammer time as possible.
            </p>
        </div>

        <div class="tile">
            <h3>Heritage</h3>
            <p>
                <em>greyd</em> closely follows the design of the excellent <em>spamd</em> from the <a target="_blank" href="http://openbsd.org">OpenBSD project</a>, and thus, implements all features of <em>spamd</em>.
            </p>
        </div>
    </div>
</section>

 <?php include('footer.php') ?>
