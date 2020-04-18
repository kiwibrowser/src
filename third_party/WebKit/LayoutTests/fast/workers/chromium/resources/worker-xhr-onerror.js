(function () {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'does-not-exist.js');
    xhr.onerror = () => { postMessage('done'); };
    xhr.send();
})();
// |xhr| isn't in scope, but it will signal pending activity
// when the GC runs, hence will be kept alive and allowed to
// run to completion.
self.gc();
