onfetch = function(event) {
  if (event.request.url.indexOf('memory-cache.json') != -1) {
    event.respondWith(
        new Response("callback({ src : 'service worker' })"));
  }
}
