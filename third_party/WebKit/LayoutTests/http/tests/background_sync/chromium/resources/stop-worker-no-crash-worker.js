self.addEventListener('sync', function(evt) {
    // Keeps this event alive.
    evt.waitUntil(new Promise(function() {}));
});

// We need this fetch handler to check the sanity of the SW using iframe().
self.addEventListener('fetch', function(evt) {
    evt.respondWith(new Response(''));
});
