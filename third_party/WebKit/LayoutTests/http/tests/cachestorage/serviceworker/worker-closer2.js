postMessage('started');
console.log('worker-close2.js starting.');
// Initializing Cache Storage before closing.
var c = caches.open('v1').then(cache => {
  return cache;
});

fetch('');
close();

// Continue to use Cache Storage after closing.
caches.open('');
c.then(cache => {
  console.log(cache.keys());
});
