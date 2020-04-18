async function setupCache() {
  var cache = await caches.open('sw-cache-1');
  console.log("Cache is ready");
  return cache.addAll([
    '/inspector-protocol/cachestorage/resources/service-worker.html',
    '/inspector-protocol/cachestorage/resources/service-worker.js'
  ]);
}

console.log("Cache is ready");
self.addEventListener('install', event => event.waitUntil(setupCache()));
