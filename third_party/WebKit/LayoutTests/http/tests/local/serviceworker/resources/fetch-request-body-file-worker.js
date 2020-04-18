self.addEventListener('fetch', function(event) {
    if (event.request.url.indexOf('NetworkFallback') != -1)
      return;
    event.respondWith(new Promise(function(resolve) {
        var headers = [];
        for (var header of event.request.headers) {
            headers.push(header);
        }
        if (event.request.url.indexOf('asText') != -1) {
          event.request.text()
            .then(function(result) {
                resolve(new Response(JSON.stringify({
                    method: event.request.method,
                    headers: headers,
                    body: result
                  })));
              });
        } else if (event.request.url.indexOf('asBlob') != -1) {
          event.request.blob()
            .then(function(result) {
                resolve(new Response(JSON.stringify({
                    method: event.request.method,
                    headers: headers,
                    body_size: result.size
                  })));
              });
        } else {
          resolve(new Response('url error:' + event.request.url));
        }
      }));
  });
