<?php

if ($_SERVER['REQUEST_METHOD'] == "OPTIONS") {
    # No preflights should happen.
    echo "FAIL";
    exit;
}

if (isset($_GET['with_credentials'])) {
    header("Access-Control-Allow-Origin: http://127.0.0.1:8000");
    header("Access-Control-Allow-Credentials: true");
} else {
    header("Access-Control-Allow-Origin: *");
}
@include("../../media/resources/serve-video.php");
?>
