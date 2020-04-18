if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test that basic EventSource cross-origin requests fail until they are allowed by the Access-Control-Allow-Origin header.");

self.jsTestIsAsync = true;

var count = 1;
var es, evt;

function runTest() {
    shouldNotThrow("es = new EventSource(\"http://127.0.0.1:8080/eventsource/resources/es-cors-basic.php?count=" + count + "\")");
    shouldBeFalse("es.withCredentials");
    es.onerror = function () {
        if (es.readyState == es.CLOSED) {
            shouldBe("es.readyState", "es.CLOSED");
            shouldBeTrue("count != 3 && count != 4");
            count++;
            setTimeout(runTest);
            return;
        }
        shouldBe("count", "4");
    };
    es.onmessage = function (arg) {
        evt = arg;
        shouldBeTrue("evt.origin.indexOf('http://127.0.0.1:8080') === 0");
        if (count == 3) {
            shouldBeTrue("evt.data == 'DATA1' && evt.lastEventId == '77'");
            count++;
            return;
        }
        shouldBe("count", "4");
        shouldBeEqualToString("evt.data", "DATA2");
        debug("DONE");
        es.close();
        finishJSTest();
    };
}
runTest();
