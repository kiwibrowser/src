importScripts('immutable-prototype.js');

if (!self.postMessage) {
  // This is a shared worker - mimic dedicated worker APIs
  onconnect = function(event) {
    event.ports[0].onmessage = function(e) {
      self.postMessage = function (msg) {
        event.ports[0].postMessage(msg);
      };
      run();
    };
  };
} else {
  run();
}

function run() {
  self.postMessage(prototypeChain(self));
}
