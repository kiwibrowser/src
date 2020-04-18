function reportResult(msg) {
    if ("opener" in self)
        self.opener.postMessage(msg, "*");
    else
        postMessage(msg);
}

new Promise(function (resolve, reject) {
    var ws = new WebSocket("ws://example.test:8880/echo");
    ws.onopen = function () {
        resolve();
    };
    ws.onmessage = function () {
        reject("Unexpected message event");
    };
    ws.onerror = function () {
        reject("Unexpected error event");
    };
    ws.onclose = function () {
        reject("Unexpected close event before open event");
    };
}).then(
    function () {
        reportResult("DONE");
    },
    function (reason) {
        reportResult("FAIL: " + reason);
    }
);
