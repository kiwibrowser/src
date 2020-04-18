if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("Test the order when there are pending open (with upgrade) and delete calls.");

indexedDBTest(null, h1OpenSuccess);
function h1OpenSuccess(evt)
{
    preamble(evt);
    evalAndLog("openWithUpgradeBlockedEventFired = false");
    evalAndLog("upgradeComplete = false");
    evalAndLog("deleteDatabaseBlockedEventFired = false");
    evalAndLog("deleteDatabaseComplete = false");

    evalAndLog("h1 = event.target.result");

    h1.onversionchange = function h1OnVersionChange(evt) {
        preamble(evt);
        shouldBe("event.target.version", "1");
        shouldBe("event.oldVersion", "1");
        shouldBe("event.newVersion", "2");

        h1.onversionchange = function h1SecondOnVersionChange(evt) {
            preamble(evt);
            testFailed('Second "versionchange" event should not be seen');
        };
    };

    debug('');
    debug("Open h2:");
    request = evalAndLog("indexedDB.open(dbname)");
    request.onblocked = unexpectedBlockedCallback;
    request.onerror = unexpectedErrorCallback;
    request.onsuccess = function h2OpenSuccess(evt) {
        preamble(evt);
        evalAndLog("h2OpenSuccess = true");
        h2 = event.target.result;

        h2.onversionchange = function h2OnVersionChange(evt) {
            preamble(evt);
            shouldBe("event.target.version", "1");
            shouldBe("event.oldVersion", "1");
            shouldBe("event.newVersion", "2");

            h2.onversionchange = function h2OnSecondVersionChange(evt) {
                preamble(evt);
                testFailed('Second "versionchange" event should not be seen');
            };
        };

        debug('');
        debug("Open h3:");
        request = evalAndLog("indexedDB.open(dbname, 2)");
        request.onerror = unexpectedErrorCallback;
        request.onsuccess = function h3OpenSuccess(evt) {
            preamble(evt);
            h3 = event.target.result;
            shouldBeTrue("upgradeComplete");
            shouldBeFalse("deleteDatabaseBlockedEventFired");
            shouldBeFalse("deleteDatabaseComplete");
            evalAndLog("h3.close()");
        };
        request.onblocked = function h3Blocked(evt) {
            preamble(evt);
            evalAndLog("openWithUpgradeBlockedEventFired = true");

            debug('');
            debug("Open h4:");
            request = evalAndLog("indexedDB.open(dbname)");
            request.onblocked = unexpectedBlockedCallback;
            request.onerror = unexpectedErrorCallback;
            request.onsuccess = function h4OpenSuccess(evt) {
                preamble(evt);
                h4 = event.target.result;
                h4.onversionchange = unexpectedVersionChangeCallback;

                shouldBeTrue("deleteDatabaseBlockedEventFired");
                shouldBeTrue("deleteDatabaseComplete");

                finishJSTest();
            };

            debug('');
            debug('Close connections to unblock previous requests:');
            evalAndLog("h1.close()");
            evalAndLog("h2.close()");
        };
        request.onupgradeneeded = function h3OnUpgradeneeded(evt) {
            preamble(evt);

            transaction = event.target.transaction;
            transaction.onabort = unexpectedAbortCallback;
            transaction.oncomplete = function transactionOnComplete(evt) {
                preamble(evt);
                evalAndLog("upgradeComplete = true");
            };
        };

        debug('... and deleteDatabase()');
        request = evalAndLog("indexedDB.deleteDatabase(dbname)");
        request.onerror = unexpectedErrorCallback;
        request.onblocked = function deleteDatabaseOnBlocked(evt) {
            preamble(evt);
            evalAndLog("deleteDatabaseBlockedEventFired = true");
        };
        request.onsuccess = function deleteDatabaseOnSuccess(evt) {
            preamble(evt);
            evalAndLog("deleteDatabaseComplete = true");
            shouldBeTrue("openWithUpgradeBlockedEventFired");
            shouldBeTrue("upgradeComplete");
            evalAndLog("deleteDatabaseBlockedEventFired = true");
        };
    };
}
