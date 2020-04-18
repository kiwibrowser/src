importScripts('/resources/testharness.js');

test(() => {
    var req = new Request('https://www.example.com/', {method: 'POST'});
    new FetchEvent('fetch', {request: req}).respondWith('foo');
  }, 'Calling respondWith should not crash');

