<?php

    $fileName = $_GET["name"];
    $type = $_GET["type"];
    $norange = $_GET["norange"];
    $cors_allow_origin = $_GET["cors_allow_origin"];

    $_GET = array();
    $_GET['name'] = $fileName;
    $_GET['type'] = $type;
    $_GET['norange'] = $norange;
    $_GET['cors_allow_origin'] = $cors_allow_origin;
    @include("./serve-video.php");

?>
