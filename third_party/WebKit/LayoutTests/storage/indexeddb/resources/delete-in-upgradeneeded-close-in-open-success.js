if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("Test that a deleteDatabase called while handling an upgradeneeded event is queued and fires its events at the right time. The close() call to unblock the delete occurs in the open request's 'success' event handler.");

function test()
{
    setDBNameFromPath();

    request = evalAndLog("indexedDB.deleteDatabase(dbname)");
    request.onsuccess = initiallyDeleted;
    request.onerror = unexpectedErrorCallback;
}

var sawUpgradeNeeded = false;
var sawOpenSuccess = false;
var sawVersionChange = false;
var sawDeleteBlocked = false;

function initiallyDeleted(evt) {
    preamble(evt);
    evalAndLog("request = indexedDB.open(dbname, 1)");
    request.onupgradeneeded = upgradeNeededCallback;
    request.onsuccess = openSuccess;
    request.onerror = unexpectedErrorCallback;
}

function upgradeNeededCallback(evt)
{
    preamble(evt);
    shouldBeFalse("sawUpgradeNeeded");
    evalAndLog("sawUpgradeNeeded = true");
    shouldBe("event.oldVersion", "0");
    shouldBe("event.newVersion", "1");

    evalAndLog("db = event.target.result");
    db.onversionchange = versionChangeCallback;
    evalAndLog("request2 = indexedDB.deleteDatabase(dbname)");
    evalAndLog("request2.onsuccess = deleteSuccessCallback");
    request2.onerror = unexpectedErrorCallback;
    request2.onblocked = deleteBlockedCallback;
}

function openSuccess(evt)
{
    preamble(evt);
    shouldBeTrue("sawUpgradeNeeded");
    shouldBeFalse("sawVersionChange");
    evalAndLog("sawOpenSuccess = true");
    evalAndLog("db = event.target.result");
    shouldBe('db.version', '1');
}

function versionChangeCallback(evt)
{
    preamble(evt);
    shouldBe("event.oldVersion", "1");
    shouldBeNull("event.newVersion");
    shouldBeTrue("sawOpenSuccess");
    evalAndLog("sawVersionChange = true");
    debug("Connection not closed at the end of 'versionchange', so 'blocked' should fire");
}

function deleteBlockedCallback(evt)
{
    preamble(evt);
    shouldBeTrue("sawVersionChange");
    shouldBeTrue("sawOpenSuccess");
    evalAndLog("sawDeleteBlocked = true");
    evalAndLog("db.close()");
}

function deleteSuccessCallback(evt)
{
    preamble(evt);
    shouldBeTrue("sawVersionChange");
    shouldBeTrue("sawDeleteBlocked");
    finishJSTest();
}

test();
