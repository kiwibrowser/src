if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test that no more message events are fired after EventSource.close() is called, even if it means discarding events that were already processed.");

self.jsTestIsAsync = true;
var es;
shouldNotThrow("es = new EventSource(\"/eventsource/resources/event-stream.php\");");
var counter = 0;
es.addEventListener('message', function (e) {
    testPassed("Got message #" + ++counter);
    if (counter > 1)
        testFailed("Handler called after the source was closed explicitly.");
    es.close();
    // Need to wait to see if we're called again.
    // event-stream.php sends a bunch of events before flushing, so if close() didn't take
    // effect we'd get a second message practically instantaneously, waiting 100ms should be ok.
    setTimeout(finishJSTest, 100);
}, false);
