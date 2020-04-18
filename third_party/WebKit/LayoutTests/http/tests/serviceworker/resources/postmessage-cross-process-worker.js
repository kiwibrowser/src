var port;
var resultport = null;
var result = null;

function send_result_if_ready() {
  if (resultport && result) {
    resultport.postMessage(result);
  }
}

self.onmessage = function(e) {
  var message = e.data;
  if ('port' in message) {
    port = message.port;
    port.onmessage = on_message;
  } else if ('done' in message) {
    result = message.done;
    send_result_if_ready();
  } else if ('resultport' in message) {
    resultport = message.resultport;
    send_result_if_ready();
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
