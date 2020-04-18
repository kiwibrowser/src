self.addEventListener('message', function(e) {
    var view = e.data;
    try {
        Atomics.wait(view, 0, 0, 0);
        postMessage("PASS: Atomics.wait is allowed in a worker.");
    } catch(e) {
        postMessage("FAIL: Atomics.wait is not allowed in a worker.");
    }
    postMessage("DONE");
});
