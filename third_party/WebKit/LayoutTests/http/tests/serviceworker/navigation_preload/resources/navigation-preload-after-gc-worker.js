
self.addEventListener('activate', e => {
    e.waitUntil(self.registration.navigationPreload.enable());
  });

self.addEventListener('fetch', e => {
    internals.collectGarbage();
    e.respondWith(new Response("hello"));
  });
