if (self.importScripts)
    importScripts("/js-test-resources/js-test.js");

description("Only .url should work, previously supported .URL should not.");

var url = "http://127.0.0.1:8000/eventsource/resources/event-stream.php";
var source = new EventSource(url);

shouldBeEqualToString("source.url", url);
shouldBeUndefined("source.URL");
finishJSTest();
