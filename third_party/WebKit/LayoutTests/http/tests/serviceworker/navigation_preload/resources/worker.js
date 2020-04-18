self.addEventListener('activate', e => {
    if (self.location.search == '?no-preload')
      return;
    e.waitUntil(self.registration.navigationPreload.enable());
  });

self.addEventListener('fetch', e => {
    if (e.request.url.endsWith('passthrough'))
      return;
    if (e.request.url.endsWith('respondWith'))
      e.respondWith(e.preloadResponse);
  });
