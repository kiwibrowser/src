<?php
header("Content-Type: text/event-stream");
$url = $_SERVER["REQUEST_URI"];
$id = $_SERVER["HTTP_LAST_EVENT_ID"];

echo "id: 77\n";
echo "retry: 300\n";
echo "data: url = " . $url . ", id = " . $id . "\n\n";
?>
