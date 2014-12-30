$(document).ready(function() {
    $(".menuControl").click(function() {
        $(".menuControl").toggle();
        $("nav ul").toggle();
        return false;
    });
});
