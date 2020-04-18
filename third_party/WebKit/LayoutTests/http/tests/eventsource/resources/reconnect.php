<?php
header("Content-Type: text/event-stream");
$lastEventId = $_SERVER['HTTP_LAST_EVENT_ID'];

if ($lastEventId)
    echo "data: $lastEventId\n\n";
else {
    # Include a non-ASCII character to test the client encodes it correctly.
    echo "id: 77☃\n";
    echo "retry: 300\n";
    echo "data: hello\n\n";
    echo "data: discarded";
}
?>
