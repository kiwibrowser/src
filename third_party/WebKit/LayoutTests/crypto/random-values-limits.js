if (self.importScripts)
    importScripts('../resources/js-test.js');

description("Tests the limits of crypto.randomValues.");

if (!self.ArrayBuffer)
    debug("This test requres ArrayBuffers to run!");

shouldBe("'crypto' in self", "true");
shouldBe("'getRandomValues' in self.crypto", "true");

try {
    var almostTooLargeArray = new Uint8Array(65536);
    var tooLargeArray = new Uint8Array(65537);

    shouldNotThrow("crypto.getRandomValues(almostTooLargeArray)");
    shouldThrow("crypto.getRandomValues(tooLargeArray)");
} catch(ex) {
    debug(ex);
}

finishJSTest();
