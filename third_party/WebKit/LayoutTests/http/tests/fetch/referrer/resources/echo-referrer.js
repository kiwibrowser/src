self.addEventListener('fetch', e => {
    if (e.request.url.match(/\/echo$/)) {
      e.respondWith(new Response(JSON.stringify({
          referrer: e.request.referrer,
          referrerPolicy: e.request.referrerPolicy,
        })));
    }
  });
