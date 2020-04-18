if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("Test IndexedDB's basics.");

indexedDBTest(prepareDatabase);
function prepareDatabase(event)
{
    trans = event.target.transaction;
    db = event.target.result;

    db.createObjectStore("objectStore");
    objectStore = trans.objectStore("objectStore");
    var req1 = objectStore.put({"key": "value"}, "object1");
    var req2 = objectStore.put({"key": "value"}, "object2");

    waitForRequests([req1, req2], createIndex);
}

function createIndex() {
    // This will asynchronously abort in the backend because of constraint failures.
    evalAndLog("objectStore.createIndex('index', 'key', {unique: true})");
    // Now immediately delete it.
    evalAndLog("objectStore.deleteIndex('index')");
    // Delete it again: backend may have asynchronously aborted the
    // index creation, but this makes sure the frontend doesn't get
    // confused and crash, or think the index still exists.
    evalAndExpectException("objectStore.deleteIndex('index')", "DOMException.NOT_FOUND_ERR", "'NotFoundError'");
    debug("Now requesting object2");
    var req3 = objectStore.get("object2");
    req3.onsuccess = unexpectedSuccessCallback;
    req3.onerror = deleteIndexAfterGetError;
    debug("now we wait.");
}

function deleteIndexAfterGetError() {
    debug("deleteIndexAfterGetError()");
    // the index should still be gone, and this should not crash.
    evalAndExpectException("objectStore.deleteIndex('index')", "11", "'InvalidStateError'");
    evalAndExpectException("objectStore.deleteIndex('index')", "11", "'InvalidStateError'");

    finishJSTest();
}
