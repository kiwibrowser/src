description("Tests adding one callback within another");

var e = document.getElementById("e");
var sameFrame;
window.requestAnimationFrame(function() {
    sameFrame = true;
}, e);
window.requestAnimationFrame(function() {
    window.requestAnimationFrame(function() {
        shouldBeFalse("sameFrame");
    }, e);
    requestAnimationFrame(function() {
        isSuccessfullyParsed();
        if (window.testRunner)
            testRunner.notifyDone();
    });
}, e);
window.requestAnimationFrame(function() {
    sameFrame = false;
}, e);

if (window.testRunner)
    testRunner.waitUntilDone();

