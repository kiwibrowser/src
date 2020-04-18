if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("readonly transaction should see the result of a previous readwrite transaction");

indexedDBTest(prepareDatabase, runTransactions);

function prepareDatabase(evt)
{
    preamble(evt);
    evalAndLog("db = event.target.result");
    evalAndLog("store = db.createObjectStore('store')");
    evalAndLog("store.put('original value', 'key')");
}

function runTransactions(evt)
{
    preamble(evt);
    evalAndLog("db = event.target.result");
    evalAndLog("transaction1 = db.transaction('store', 'readwrite')");
    transaction1.onabort = unexpectedAbortCallback;
    evalAndLog("transaction2 = db.transaction('store', 'readonly')");
    transaction2.onabort = unexpectedAbortCallback;

    evalAndLog("request = transaction1.objectStore('store').put('new value', 'key')");
    request.onerror = unexpectedErrorCallback;

    evalAndLog("request2 = transaction2.objectStore('store').get('key')");
    request2.onerror = unexpectedErrorCallback;
    request2.onsuccess = function checkResult(evt) {
        preamble(evt);
        shouldBeEqualToString('request2.result', 'new value');
        db.close();
        finishJSTest();
    };
}
