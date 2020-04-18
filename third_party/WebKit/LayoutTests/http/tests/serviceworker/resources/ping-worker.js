var counter = 0;
self.onmessage = function(event) {
  event.data.port.postMessage(counter++);
};
