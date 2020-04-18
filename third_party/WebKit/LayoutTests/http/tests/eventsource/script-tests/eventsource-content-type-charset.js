if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Test EventSource with an event-stream with a Content-Type with a charset is still recognized.");
// Test for bug https://bugs.webkit.org/show_bug.cgi?id=45372

self.jsTestIsAsync = true;

var es;

function shouldGetMessage() {
    debug("Content type: " + es.contentType);
    shouldBeFalse("!!es.sawError");
    shouldBeTrue("es.sawMessage");
    shouldBeTrue("es.sawOpen");
}

function shouldFail() {
    debug("Content type: " + es.contentType);
    shouldBeTrue("es.sawError");
    shouldBeFalse("!!es.sawMessage");
    shouldBeFalse("!!es.sawOpen");
}

var i = 0;
var contentTypes = [ 'text/event-stream; charset=UTF-8',
                     'text/event-stream; charset=windows-1251',
                     'text/event-stream; charset=utf-8',
                     'text/event-stream; charset="UTF-8"',
                     'text/event-stream-foobar;'
                   ];

var expectedResultCallback = [ shouldGetMessage,
                               shouldFail,
                               shouldGetMessage,
                               shouldGetMessage,
                               shouldFail
                             ];

function openListener(evt)
{
    evt.target.sawOpen = true;
}

function messageListener(evt)
{
    evt.target.sawMessage = true;
    evt.target.successCallback(evt.target);
    evt.target.close();
    next();
}

function errorListener(evt)
{
    evt.target.sawError = true;
    evt.target.successCallback(evt.target);
    evt.target.close();
    next();
}

function startRequest()
{
    shouldNotThrow("es = new EventSource(\"/eventsource/resources/response-content-type-charset.php?contentType=" + escape(contentTypes[i]) + "\")");
    es.onopen = openListener;
    es.onmessage = messageListener;
    es.onerror = errorListener;
    es.successCallback = expectedResultCallback[i];
    es.contentType = contentTypes[i];
    ++i;
}

function next()
{
    if (i >= contentTypes.length) {
        finishJSTest();
        return;
    }
    startRequest();
}
startRequest();
