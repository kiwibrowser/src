if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test EventSource with non-HTTP protocol schemes in the URL.");

self.jsTestIsAsync = true;

var es, count = 0;

new EventSource("ftp://127.0.0.1").onerror =
new EventSource("file:///etc/motd").onerror =
new EventSource("localhost:8080/").onerror =
new EventSource("tel:1234").onerror = function () {
    es = this;
    shouldBe("es.readyState", "EventSource.CLOSED");
    if (count++ == 3)
        finishJSTest();
};
