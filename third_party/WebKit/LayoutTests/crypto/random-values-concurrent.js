// Compute some random values and reply with it.
var sample = new Uint8Array(100);
crypto.getRandomValues(sample);
self.postMessage(sample);
