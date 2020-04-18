if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
}

window.addEventListener("message", function(message) {
    testRunner.notifyDone();
}, false);

// This is needed because isolated worlds are not reset between test runs and a
// previous test's CSP may interfere with this test. See
// https://crbug.com/415845.
testRunner.setIsolatedWorldContentSecurityPolicy(1, '');

function test() {
    function setFontFace(num) {
        var style = document.createElement("style");
        style.innerText = "@font-face { font-family: 'remote'; src: url(/resources/Ahem.ttf); }";
        document.getElementById('body').appendChild(style);
        window.postMessage("next", "*");
    }

    alert("Bypass main world's CSP with font-face.");
    testRunner.setIsolatedWorldContentSecurityPolicy(1, "font-src 'self'");
    testRunner.evaluateScriptInIsolatedWorld(1, String(eval("setFontFace")) + "\nsetFontFace(1);");
}

document.addEventListener('DOMContentLoaded', test);
