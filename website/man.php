<?php $active = 'docs'; include('header.php'); ?>

<?php
     $manuals = array('greyd', 'greylogd', 'greydb', 'greyd-setup', 'greyd.conf');
     $name = $_REQUEST['name'];
     if(!in_array($name, $manuals)) {
         header('Location: docs.php');
         exit;
     }
     $section = ($name == 'greyd.conf' ? 5 : 8);
?>

<section class="banner">
    <div class="content">
        <h2>
            <?php echo ucfirst($name) ?> Documentation
        </h2>
    </div>
</section>

<br />

<section>
    <iframe src="<?php echo "$name.$section.html" ?>"></iframe>
</section>

<?php include('footer.php') ?>
