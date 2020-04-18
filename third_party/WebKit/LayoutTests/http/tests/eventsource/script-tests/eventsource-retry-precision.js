if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test EventSource retry time precision.");

self.jsTestIsAsync = true;

var timeoutId = setTimeout(function () {
    testPassed("did not immediately reconnect");
    end();
}, 500);

var count = 0;
var es = new EventSource("resources/precise-retry.asis");
es.onopen = function () {
    if (count++ != 2)
        return;
    clearTimeout(timeoutId);
    testFailed("reconnected too soon");
    end();
};

function end() {
    es.close();
    debug("DONE");
    finishJSTest();
}
