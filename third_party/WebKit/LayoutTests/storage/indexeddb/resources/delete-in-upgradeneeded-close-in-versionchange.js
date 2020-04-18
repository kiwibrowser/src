if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("Test that a deleteDatabase called while handling an upgradeneeded event is queued and fires its events at the right time. The close() call to unblock the delete occurs in the connection's 'versionchange' event handler.");

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

function initiallyDeleted(evt) {
    preamble(evt);
    evalAndLog("request = indexedDB.open(dbname, 1)");
    request.onupgradeneeded = upgradeNeededCallback;
    request.onsuccess = openSuccessCallback;
}

function upgradeNeededCallback(evt)
{
    preamble(evt);
    shouldBeFalse("sawUpgradeNeeded");
    shouldBeFalse("sawOpenSuccess");
    evalAndLog("sawUpgradeNeeded = true");
    shouldBe("event.oldVersion", "0");
    shouldBe("event.newVersion", "1");

    evalAndLog("db = event.target.result");
    db.onversionchange = versionChangeCallback;
    request2 = evalAndLog("deleteRequest = indexedDB.deleteDatabase(dbname)");
    evalAndLog("request2.onsuccess = deleteSuccessCallback");
    request2.onerror = unexpectedErrorCallback;
    request2.onblocked = unexpectedBlockedCallback;;
}

function versionChangeCallback(evt) {
    preamble(evt);
    shouldBeTrue("sawOpenSuccess");
    shouldBe("event.oldVersion", "1");
    shouldBeNull("event.newVersion");
    evalAndLog("sawVersionChange = true");
    evalAndLog("db.close()");
}

function openSuccessCallback(evt)
{
    preamble(evt);
    shouldBeTrue("sawUpgradeNeeded");
    shouldBeFalse("sawVersionChange");
    evalAndLog("sawOpenSuccess = true");
}

function deleteSuccessCallback(evt)
{
    preamble(evt);
    shouldBeTrue("sawVersionChange");
    shouldBeTrue("sawUpgradeNeeded");
    finishJSTest();
}

test();
