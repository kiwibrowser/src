<?php
$allowOrigin = $_GET['allow'];
if ($allowOrigin == "true") {
    header("Access-Control-Allow-Origin: *");
}
header('Content-Type: application/javascript');
?>
new Promise(function(resolve, reject) { reject(42); });
