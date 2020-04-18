self.onfetch = function(e) {
  if (!/\.txt.*/.test(e.request.url))
    return;
  e.respondWith(
    self.caches.open('ignore-search')
      .then(function(cache) {
          return cache.add(e.request);
        })
      .then(function() {
          return new Response(e.request.url);
        })
  );
};
