<?php
  header('Cache-control: no-store');
  header('Content-Security-Policy: referrer origin-when-cross-origin');
  header('Content-Type: application/javascript');
?>

self.addEventListener('fetch', e => e.respondWith(fetch(e.request)));
