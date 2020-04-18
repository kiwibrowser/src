self.addEventListener('fetch', evt => {
    // Retruns an empty response to resolve with_iframe() in the test page.
    evt.respondWith(new Response(''));
    // Keeps this event alive.
    evt.waitUntil(new Promise(function() {}));
  });
