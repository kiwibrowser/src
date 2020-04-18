if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test that EventSource cross-origin requests with credentials fail until the correct CORS headers are sent.");

self.jsTestIsAsync = true;

var count = 1;
var es, evt;

function runTest() {
    shouldNotThrow("es = new EventSource(\"http://127.0.0.1:8080/eventsource/resources/es-cors-credentials.php?count=" + count + "\", {'withCredentials': true})");
    shouldBeTrue("es.withCredentials");
    es.onerror = function () {
        if (es.readyState == es.CLOSED) {
            shouldBeTrue("count != 4 && count != 5");
            count++;
            setTimeout(runTest);
            return;
        }
        shouldBe("count", "5");
    };
    es.onmessage = function (arg) {
        evt = arg;
        shouldBeTrue("evt.origin.indexOf('http://127.0.0.1:8080') === 0");
        if (count == 4) {
            shouldBeEqualToString("evt.data", "DATA1");
            shouldBeEqualToString("evt.lastEventId", "77");
            count++;
            return;
        }
        shouldBe("count", "5");
        shouldBeEqualToString("evt.data", "DATA2");
        es.close();
        finishJSTest();
    };
}
runTest();
