var ws = new WebSocket("ws://127.0.0.1:8880/echo");
var errorCalled = false;
ws.onopen = function () {
    postMessage("FAIL: Unexpected open event on ws");
};
ws.onmessage = function () {
    postMessage("FAIL: Unexpected message event on ws");
};
ws.onerror = function () {
    errorCalled = true;
};
ws.onclose = function () {
    if (errorCalled)
        postMessage("DONE");
    else
        postMessage("FAIL: Error event was not dispatched");
};
ws.close();
