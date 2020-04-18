self.onerror = function (message, filename, lineno, colno, error) {
    postMessage({ 'message': message, 'filename': filename, 'lineno': lineno, 'colno': colno, 'error': error });
};

var differentRedirectOrigin = "/resources/redirect.php?url=http://localhost:8000/workers/resources/worker-importScripts-throw.js";
importScripts(differentRedirectOrigin)
