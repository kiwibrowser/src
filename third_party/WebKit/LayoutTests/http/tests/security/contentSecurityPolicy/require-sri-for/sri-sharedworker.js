onconnect = function(e) {
  var port = e.ports[0];
  port.postMessage("shared ping");
}

