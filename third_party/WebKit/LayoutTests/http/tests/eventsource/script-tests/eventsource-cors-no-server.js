if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test that EventSource tries to reconnect if there's no server response when making cross-origin requests.");

self.jsTestIsAsync = true;

var count = 0;
var hosts = ["http://127.0.0.1:12345/event-stream", "http://localhost:54321/event-stream"];

var es;

function runTest() {
    shouldNotThrow("es = new EventSource(\"" + hosts[count] + "\")");
    es.onerror = function () {
        if (es.readyState == es.CONNECTING) {
            testPassed("got error event and readyState is CONNECTING");
            es.close();
        } else
            shouldBeTrue("es.readyState !== es.CLOSED");
        if (++count == hosts.length) {
            finishJSTest();
            return;
        }
        setTimeout(runTest);
    };
}
runTest();
