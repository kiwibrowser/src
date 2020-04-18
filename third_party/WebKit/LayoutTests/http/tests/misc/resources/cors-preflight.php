<?php
header("Access-Control-Allow-Origin: *");
header("Access-Control-Max-Age: 0");
header("Timing-Allow-Origin: *");

if ($_SERVER["REQUEST_METHOD"] == "OPTIONS") {
    header("Access-Control-Allow-Headers: X-Require-Preflight");
    ob_start();
}                                     
?>
<!DOCTYPE html>
<title>CORS preflight test</title>
If this script is accessed with the header X-Require-Preflight then the
browser will send a preflight request. Otherwise it won't.

<?php
if ($_SERVER["REQUEST_METHOD"] == "OPTIONS") {
    # Discard the body.
    ob_end_clean();
}
?>