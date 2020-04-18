import('./empty-worker.js')
    .catch(e => self.postMessage({ name: e.name, message: e.message }));
