function wait(delay) {
  return new Promise(resolve => setTimeout(resolve, delay));
}

self.addEventListener('fetch', e => {
    self.gc();
    e.respondWith(wait(10).then(() => {
        self.gc();
        return fetch(e.request);
      }));
  });
