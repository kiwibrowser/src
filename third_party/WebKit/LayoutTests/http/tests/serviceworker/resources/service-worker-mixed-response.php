<?php
$position = $_GET["position"];
header('HTTP/1.0 206 Partial Content');
header('Content-Type: audio/ogg');
# 12983 is the file size of media/content/silence.oga.
header('Content-Range: bytes ' . $position . '-' . $position . '/12983');
header('Access-Control-Allow-Origin: *');
echo substr('Ogg', $position, 1);
?>
