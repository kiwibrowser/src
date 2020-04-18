// A service worker for use with UseCounter tests. It uses a feature
// when asked via postMessage.
self.addEventListener('message', e => {
    if (e.data == 'use-frameType') {
      self.clients.matchAll().then(my_clients => {
          my_clients[0].frameType;
        });
    }
  });
