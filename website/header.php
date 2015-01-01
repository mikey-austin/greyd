<!DOCTYPE html>

<html>
    <head>
        <title>Greyd - spam deferral daemon</title>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <link rel="stylesheet" href="css/screen.css" />
        <script type="text/javascript" src="https://code.jquery.com/jquery-1.11.2.min.js"></script>
        <script type="text/javascript" src="js/site.js"></script>
    </head>
    <body>
        <header class="group">
            <h1 class="logo">
                <a href="index.php">
                    grey<span>d</span>
                </a>
            </h1>

            <a class="menuControl expand" href="#">+</a>
            <a class="menuControl collapse" href="#">-</a>

            <nav>
                <ul>
                    <li <?php if(isset($active) && $active == 'docs'): ?>class="active"<?php endif; ?>>
                        <a href="docs.php">docs</a>
                    </li>
                    <li <?php if(isset($active) && $active == 'downloads'): ?>class="active"<?php endif; ?>>
                        <a href="downloads.php">downloads</a>
                    </li>
                    <li <?php if(isset($active) && $active == 'code'): ?>class="active"<?php endif; ?>>
                        <a href="code.php">code</a>
                    </li>
                </ul>
            </nav>
        </header>

        <main>
