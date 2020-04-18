var count = 0;

onconnect = function(event) {
  event.ports[0].postMessage(++count);
};
