onconnect = event => {
  const port = event.ports[0];
  import('./empty-worker.js')
      .catch(e => port.postMessage({ name: e.name, message: e.message }));
};
