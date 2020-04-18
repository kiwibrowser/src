self.onerror = function (message, filename, lineno, colno, error) {
    postMessage({ 'message': message, 'filename': filename, 'lineno': lineno, 'colno': colno, 'error': error });
};

importScripts('/workers/resources/worker-importScripts-throw.js');
