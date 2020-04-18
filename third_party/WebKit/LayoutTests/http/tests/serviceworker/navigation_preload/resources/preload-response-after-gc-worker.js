
self.addEventListener('activate', e => {
    e.waitUntil(self.registration.navigationPreload.enable());
  });

self.addEventListener('fetch', e => {
    internals.collectGarbage();
    // Sleeps 100 ms.
    var end = Date.now() + 100;
    while (Date.now() < end);
    e.respondWith(new Response("hello"));
  });
