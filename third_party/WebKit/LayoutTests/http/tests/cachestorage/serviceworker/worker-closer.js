postMessage('started');
console.log('worker-close.js starting.');
fetch('');
close();
// Touching Cache Storage only after closing.
caches.open('');
