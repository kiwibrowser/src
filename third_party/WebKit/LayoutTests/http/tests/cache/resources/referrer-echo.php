<?php
header("Last-Modified: " . gmdate(DATE_RFC1123, time()));
header("Content-Type: text/javascript");
echo "var referrer = '" . (isset($_SERVER["HTTP_REFERER"]) ? $_SERVER["HTTP_REFERER"] : "none") . "';";
?>
