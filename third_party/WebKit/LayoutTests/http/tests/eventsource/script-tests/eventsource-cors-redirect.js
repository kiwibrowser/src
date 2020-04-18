if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test that basic EventSource cross-origin requests fail on redirect.");

self.jsTestIsAsync = true;

var es;

shouldNotThrow("es = new EventSource(\"/resources/redirect.php?code=307&url=http://127.0.0.1:8080/eventsource/resources/es-cors-basic.php\")");
es.onerror = function() {
    shouldBe("es.readyState", "EventSource.CLOSED");
    finishJSTest();
};
