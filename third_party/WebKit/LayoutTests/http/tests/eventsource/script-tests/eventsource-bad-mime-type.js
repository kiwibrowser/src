if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test EventSource with an event-stream with incorrect mime-type.");

self.jsTestIsAsync = true;

function end() {
    es.close();
    finishJSTest();
}

var es = new EventSource("resources/bad-mime-type.asis");
es.onopen = function (evt) {
    testFailed("got unexpected open event");
    end();
};

es.onmessage = function (evt) {
    testFailed("got unexpected message event");
    end();
};

es.onerror = function () {
    shouldBe("es.readyState", "es.CLOSED");
    end();
};
