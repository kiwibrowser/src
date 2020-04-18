if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test EventSource text/event-stream parsing.");

self.jsTestIsAsync = true;

var count = -1;
var es, evt;

shouldNotThrow("es = new EventSource(\"resources/event-stream.php\");");
es.onopen = function (evt) {
    testPassed("got open event" + (!evt.data ? "" : " from server"));
};

es.onmessage = function (arg) {
    evt = arg;
    switch(count++) {
    case -1:
        shouldBeEqualToString("evt.data", "\n\n");
        break;
    case 0:
        shouldBeEqualToString("evt.data", "simple");
        break;
    case 1:
        shouldBeEqualToString("evt.data", "spanning\nmultiple\n\nlines\n");
        break;
    case 2:
        shouldBeEqualToString("evt.data", "id is 1");
        shouldBeEqualToString("evt.lastEventId", "1");
        break
    case 3:
        shouldBeEqualToString("evt.data", "id is still 1");
        shouldBeEqualToString("evt.lastEventId", "1");
        break;
    case 4:
        shouldBeEqualToString("evt.data", "no id");
        shouldBeEqualToString("evt.lastEventId", "");
        break;
    case 5:
        shouldBeEqualToString("evt.data", "a message event with the name \"message\"");
        break;
    case 6:
        shouldBeEqualToString("evt.data", "a line ending with crlf\na line with a : (colon)\na line ending with cr");
        break;
    default:
        testFailed("got unexpected event");
        es.close();
    }
};

es.onerror = function () {
    es.close();

    shouldBe("count", "7");
    debug("DONE");
    finishJSTest();
};
