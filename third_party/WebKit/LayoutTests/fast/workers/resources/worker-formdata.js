importScripts('../../../resources/js-test.js');

description("Test FormData interface object");

shouldBeDefined("FormData");
shouldBe("FormData.length", "0");

var formData = new FormData();

shouldBeNonNull("formData");

shouldBeTrue("FormData.prototype.hasOwnProperty('append')");
shouldNotThrow("formData.append('key', 'value');");
var blob = new Blob([]);
shouldBeNonNull("blob");
shouldNotThrow("formData.append('key', blob);");
shouldNotThrow("formData.append('key', blob, 'filename');");
shouldThrow("postMessage(formData)");
finishJSTest();
