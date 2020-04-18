self.postMessage("Test started.");
// The test will create 3 timeouts with their intervals decreasing.
// If the timeouts execute in order then the test is PASS.
self.addEventListener('message', function(e) {
    var t1 = setTimeout(function () { postMessage(1); }, 5);
    setTimeout(function () { postMessage(2); postMessage("DONE"); }, 10);
    clearTimeout(t1);
}, false);