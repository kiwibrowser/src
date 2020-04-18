if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("Test that deleteDatabase is not blocked when connections close in on versionchange callback");

function test() {
    setDBNameFromPath();

    request = evalAndLog("indexedDB.open(dbname)");
    request.onblocked = unexpectedBlockedCallback;
    request.onerror = unexpectedErrorCallback;
    request.onsuccess = function openOnSuccess(evt) {
        preamble(evt);
        evalAndLog("h = event.target.result");

        h.onversionchange = function onVersionChange(evt) {
            preamble(evt);
            shouldBe("event.target.version", "1");
            shouldBe("event.oldVersion", "1");
            shouldBeNull("event.newVersion");
            evalAndLog("h.close()");
        };

        request = evalAndLog("indexedDB.deleteDatabase(dbname)");
        request.onerror = unexpectedErrorCallback;
        request.onblocked = unexpectedBlockedCallback;
        request.onsuccess = function deleteDatabaseOnSuccess(evt) {
            preamble(evt);
            testPassed("blocked event was not fired");
            finishJSTest();
        };
    };
}

test();
