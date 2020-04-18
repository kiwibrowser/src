importScripts('../../../../resources/js-test.js');

var file = new File(['hello'], 'hello.txt', {lastModified: new Date(1234567890)});

// This should not crash.
shouldBe("file.lastModifiedDate.valueOf()", "1234567890");
finishJSTest();
