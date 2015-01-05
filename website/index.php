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
                <em>greyd</em> runs on <strong>GNU/Linux</strong> systems (with the <em>netfilter</em> firewall driver), and <strong>BSD</strong> (with the <em>pf</em> firewall driver).
            </p>
            <p>
                <em>greyd</em> closely integrates with the firewall, and provides an abstract interface to theoretically support any appropriate firewall system.
            </p>
            <p>
                One of the goals of the <em>greyd</em> project is to support a wide variety of systems, firewalls and databases.
            </p>

            <h3>Heritage</h3>
            <p>
                <em>greyd</em> closely follows the design of the excellent <em>spamd</em> from the <a target="_blank" href="http://openbsd.org">OpenBSD project</a>, and thus, implements all features of <em>spamd</em>.
            </p>
        </div>

        <div class="tile">
            <h3>Blacklisting</h3>
            <p>
                Blacklists may also be loaded into <em>greyd</em> (via <a href="man.php?name=greyd-setup">greyd-setup</a>). If <em>greyd</em> finds that a host is on one or more blacklists, it will reply <em>very slowly</em> in order to waste as much spammer time as possible.
            </p>

            <h3>Spam-trapping</h3>
            <p>
                <em>greyd</em> provides multiple ways to <em>trap</em> spammers (aka <em>greytrapping</em>):
            </p>
            <ul>
                <li>if the spammer sends mail to a designated spamtrap address (setup via <a href="man.php?name=greydb">greydb</a>)</li>
                <li>if the spammer sends mail to a non-existant address</li>
                <li>if the spammer sends mail to a designated backup MX out of order</li>
            </ul>
            <p>
                Trapped hosts are treated in the same fashion as blacklisted hosts.
            </p>
        </div>
    </div>
</section>

 <?php include('footer.php') ?>
