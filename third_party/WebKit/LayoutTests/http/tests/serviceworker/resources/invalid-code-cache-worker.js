self.addEventListener('install', event => {
  event.waitUntil(caches.open('test').then(
      cache =>
      cache.put(
          new Request('./invalid.js'),
          new Response('(,);',
          {headers: [['content-type', 'text/javascript']]}))));
});