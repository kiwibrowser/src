<?php
    $accept = explode(",", $_SERVER["HTTP_ACCEPT"]);
    $accept_images = false;
    foreach ($accept as $a) {
        if (strpos($a, ";q=")) {
            # Skip quality annotation.
            $a = substr($a, 0, strpos($a, ";q="));
        }
        if ($a == "*/*" || $a == "image/*" || $a == "image/jpg") {
            header("Content-Type: image/jpg");
            header("Cache-Control: no-store");
            header("Connection: close");

            $fn = fopen("compass.jpg", "r");
            fpassthru($fn);
            fclose($fn);
            exit;
        }
    }
?>
