self.addEventListener('activate', event => {
    event.waitUntil(self.registration.navigationPreload.enable());
  });

self.addEventListener('fetch', event => {
    event.respondWith(
      event.preloadResponse
          .then(response => response.text())
          .then(text =>
            new Response(
              JSON.stringify({timingEntries: performance.getEntries()}),
              {headers: {'Content-Type': 'text/html'}})));
  });
