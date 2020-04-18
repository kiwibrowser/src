if (this.importScripts)
    importScripts('../../../resources/js-test.js');

description("Tests that atob() / btoa() functions are exposed to workers");

shouldBeTrue("typeof atob === 'function'");
shouldBeTrue("typeof btoa === 'function'");

finishJSTest();
