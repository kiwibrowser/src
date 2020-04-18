<?php
header('Cache-Control: max-age=86400');
header('Content-type: text/javascript');
?>
self.addEventListener('fetch', function(event) {
    event.respondWith(new Response('hello'));
});
