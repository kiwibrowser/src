onmessage = event => {
  const source = event.source;
  import('empty-worker.js')
      .catch(e => source.postMessage({ name: e.name, message: e.message }));
};
