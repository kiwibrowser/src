onconnect = e => {
  var port = e.ports[0];
  port.onmessage = (e) => {
    if (e.data.type == 'COUNT_FEATURE')
      internals.countFeature(e.data.feature);
    else if (e.data.type == 'COUNT_DEPRECATION')
      internals.countDeprecation(e.data.feature);
  };
  port.postMessage('CONNECTED');
}
