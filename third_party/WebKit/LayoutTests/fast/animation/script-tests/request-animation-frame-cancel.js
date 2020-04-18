description("This tests cancelling a requestAnimationFrame callback");

var callbackFired = false;
var e = document.getElementById("e");
var id = window.requestAnimationFrame(function() {
    callbackFired = true;
}, e);

window.cancelAnimationFrame(id);

window.requestAnimationFrame(function() {
    shouldBeFalse("callbackFired");
    isSuccessfullyParsed();
    if (window.testRunner)
        testRunner.notifyDone();
});

if (window.testRunner)
  testRunner.waitUntilDone();
