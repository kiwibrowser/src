'use strict';

// Adapter for Mozilla's getAll tests to Blink's js-test.

var jsTestIsAsync = true;

// Moz tests are still prefixed
IDBObjectStore.prototype.mozGetAll = IDBObjectStore.prototype.getAll;
IDBObjectStore.prototype.mozGetAllKeys = IDBObjectStore.prototype.getAllKeys;
IDBIndex.prototype.mozGetAll = IDBIndex.prototype.getAll;
IDBIndex.prototype.mozGetAllKeys = IDBIndex.prototype.getAllKeys;

function errorHandler(e) {
    testFailed("Unexpected error: " + e.type + " - " + e.message);
    finishJSTest();
}
function ok(t, message) {
    if (!t) {
        testFailed(message);
        finishJSTest();
    } else {
        testPassed(message);
    }
}
function is(a, b, message) {
    ok(Object.is(a, b), message);
}
function grabEventAndContinueHandler(e) {
    testGenerator.next(e);
}
function executeSoon(f) {
    setTimeout(f, 0);
}
function continueToNextStepSync() {
    testGenerator.next();
}
function finishTest() {
    finishJSTest();
}
function unexpectedSuccessHandler(e) {
    ok(false, "Unexpected success");
}
function info(message) {
    debug(message);
}
testGenerator.next();
