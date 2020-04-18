self.onerror = function (message, filename, lineno, colno, error) {
    postMessage({ 'message': message, 'filename': filename, 'lineno': lineno, 'colno': colno, 'error': error });
};

importScripts('http://localhost:8000/workers/resources/worker-importScripts-throw.js');
