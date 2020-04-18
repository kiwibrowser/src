<?php
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
header('Content-type: text/javascript');
echo "// " . microtime() . "\n";
?>
self.addEventListener('fetch', function(event) {
    event.respondWith(new Response('Hello'));
});
