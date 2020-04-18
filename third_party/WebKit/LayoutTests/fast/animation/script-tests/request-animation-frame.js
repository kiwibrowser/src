description("Tests basic use of requestAnimationFrame");

var e = document.getElementById("e");
var callbackInvoked = false;
window.requestAnimationFrame(function() {
    callbackInvoked = true;
    shouldBeTrue("callbackInvoked");
    isSuccessfullyParsed();
    if (window.testRunner)
        testRunner.notifyDone();
}, e);

if (window.testRunner)
    testRunner.waitUntilDone();
