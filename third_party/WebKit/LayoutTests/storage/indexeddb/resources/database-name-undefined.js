if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

description("Test IndexedDB undefined as record value");

function test()
{
    shouldThrow("indexedDB.open();");
    finishJSTest();
}

test();
