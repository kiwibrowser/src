<?php
sleep(intval($_GET['timeout']));
header("HTTP/1.1 404 Not Found");
header("Content-Type: text/javascript");
echo("testFailed('script should not run');");
?>
