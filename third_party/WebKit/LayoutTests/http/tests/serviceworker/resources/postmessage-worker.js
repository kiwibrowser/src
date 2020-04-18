var port;

self.onmessage = function(e) {
  var message = e.data;
  if ('port' in message) {
    port = message.port;
    port.onmessage = on_message;
  }
};

function on_message(e) {
  var message = e.data;
  if ('value' in message) {
    port.postMessage('Acking value: ' + message.value);
  } else if ('done' in message) {
    port.postMessage('quit');
  }
}
