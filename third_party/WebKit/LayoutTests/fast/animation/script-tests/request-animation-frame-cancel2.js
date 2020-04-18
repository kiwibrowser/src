description("Tests one requestAnimationFrame callback cancelling a second");

var e = document.getElementById("e");
var secondCallbackId;
var callbackFired = false;
var cancelFired = false;

window.requestAnimationFrame(function() {
    cancelFired = true;
    window.cancelAnimationFrame(secondCallbackId);
}, e);

secondCallbackId = window.requestAnimationFrame(function() {
    callbackFired = true;
}, e);

requestAnimationFrame(function() {
    shouldBeFalse("callbackFired");
    shouldBeTrue("cancelFired");
    isSuccessfullyParsed();
    if (window.testRunner)
        testRunner.notifyDone();
});

if (window.testRunner)
    testRunner.waitUntilDone();
