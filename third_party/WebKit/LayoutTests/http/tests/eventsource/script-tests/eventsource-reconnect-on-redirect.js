if (self.importScripts)
    importScripts('/js-test-resources/js-test.js');

description('Test EventSource reconnect after end of event stream.');

self.jsTestIsAsync = true;

var retryTimeout;

function checkReadyState(es, desiredState) {
    shouldBe('es.readyState', 'es.' + desiredState);
}

var errorCount = 0;
var es = new EventSource('/eventsource/resources/redirect.php');

checkReadyState(es, 'CONNECTING');

es.onopen = () => checkReadyState(es, 'OPEN');
// We expect two messages are received.
//  first:  "url=.../echo-url.php?id=, id = " because we don't set
//          Last-Event-ID.
//  second: "url=.../echo-url-php?id=, id = 77" because we set Last-Event-ID
//          to 77, but we don't go through redirect.php this time.
// If we wrongly used the original URL for reconnect, the url would include 77
// in the second message.
es.onmessage = (e) => debug(`got a message: ${e.data}`);

function timeout() {
    es.close();
    testFailed('reconnect timed out');
    finishJSTest();
}

es.onerror = () => {
    errorCount++;
    if (errorCount < 2) {
        checkReadyState(es, 'CONNECTING');
        retryTimeout = setTimeout(timeout, 1000);
        return;
    }
    clearTimeout(retryTimeout);
    es.close();
    checkReadyState(es, 'CLOSED');
    shouldBeEqualToString('es.url', 'http://127.0.0.1:8000/eventsource/resources/redirect.php');
    finishJSTest();
};

