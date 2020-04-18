self.addEventListener('activate', e => {
    e.waitUntil(self.registration.navigationPreload.enable());
  });

self.addEventListener('fetch', e => {
    setTimeout(_ => { internals.collectGarbage(); }, 0);
    e.respondWith(
        e.preloadResponse
          .then(response => { return response; }));
  });
