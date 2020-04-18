let requests = [];

self.addEventListener('fetch', (event) => {
    requests.push({
        url: event.request.url,
        mode: event.request.mode
      });
  });

self.addEventListener('message', (event) => {
    event.data.port.postMessage(requests);
    requests = [];
  });
