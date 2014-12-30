$(document).ready(function() {
    $(".menuControl").click(function() {
        $(".menuControl").toggle();
        $("nav ul").toggle();
        return false;
    });

    $("iframe").load(function () {
        $(this).height($(this).contents().find("body").height() + 40);
        $(this).width($(this).contents().find("body").width());
    });
});
