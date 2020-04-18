if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test EventSource reconnect after end of event stream.");

self.jsTestIsAsync = true;

var retryTimeout;

function checkReadyState(es, desiredState) {
    shouldBe("es.readyState", "es." + desiredState);
}

var errCount = 0;
var es = new EventSource("/eventsource/resources/reconnect.php");

checkReadyState(es, "CONNECTING");

es.onopen = function (evt) {
    checkReadyState(es, "OPEN");
};

var evt;
es.onmessage = function (arg) {
    evt = arg;
    if (errCount)
        shouldBeEqualToString("evt.data", "77\u2603");
    shouldBeEqualToString("evt.lastEventId", "77\u2603");
};

es.onerror = function () {
    errCount++;
    if (errCount < 2) {
        checkReadyState(es, "CONNECTING");
        retryTimeout = setTimeout(end, 1000);
        return;
    }
    clearTimeout(retryTimeout);
    retryTimeout = null;
    end();
};

function end() {
    es.close();
    if (retryTimeout)
        testFailed("did not reconnect in time");
    else
        checkReadyState(es, "CLOSED");

    finishJSTest();
}
