if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("Test that deleteDatabase is delayed if a VERSION_CHANGE transaction is running");

indexedDBTest(prepareDatabase, onOpenSuccess);
function prepareDatabase(evt)
{
    preamble(evt);
    evalAndLog("sawVersionChange = false");
    evalAndLog("upgradeTransactionComplete = false");
    evalAndLog("h = event.target.result");

    h.onversionchange = function onVersionChange(evt) {
        preamble(evt);
        shouldBe("event.target.version", "1");
        shouldBe("event.oldVersion", "1");
        shouldBeNull("event.newVersion");
        evalAndLog("sawVersionChange = true");
        debug("Connection is not closed, so 'blocked' should fire");
    };

    transaction = event.target.transaction;
    transaction.oncomplete = function transactionOnComplete(evt) {
        preamble(evt);
        evalAndLog("upgradeTransactionComplete = true");
    };

    request = evalAndLog("indexedDB.deleteDatabase(dbname)");
    request.onerror = unexpectedErrorCallback;
    request.onblocked = function deleteDatabaseOnBlocked(evt) {
        preamble(evt);
        shouldBeTrue("sawVersionChange");
        evalAndLog("h.close()");
    };
    request.onsuccess = function deleteDatabaseOnSuccess(evt) {
        preamble(evt);
        shouldBeTrue("upgradeTransactionComplete");
        finishJSTest();
    };
}

function onOpenSuccess(evt)
{
    preamble(evt);
    evalAndLog("h = event.target.result");
}
