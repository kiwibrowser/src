if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test that EventSource discards event data if there is no newline before eof.");

self.jsTestIsAsync = true;

var count = 1;

var es = new EventSource("/eventsource/resources/es-eof.php");

es.onerror = function () {
    if (count++ == 3) {
        es.close();
        finishJSTest();
    }
};

var evt;

es.onmessage = function (arg) {
    evt = arg;
    if (evt.data == ("DATA" + count)) {
        shouldBeEqualToString("evt.type", "message");
        shouldBeEqualToString("evt.data", "DATA" + count);
        shouldBeEqualToString("evt.lastEventId", count.toString());
        return;
    }
    shouldBeEqualToString("evt.type", "msg");
    shouldBeEqualToString("evt.data", "DATA");
    shouldBeEqualToString("evt.lastEventId", "3.1");
};
es.addEventListener("msg", es.onmessage);
