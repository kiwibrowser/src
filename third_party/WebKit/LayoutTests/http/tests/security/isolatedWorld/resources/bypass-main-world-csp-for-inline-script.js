if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
}

tests = 4;
window.addEventListener("message", function(message) {
    tests -= 1;
    test();
}, false);

// This is needed because isolated worlds are not reset between test runs and a
// previous test's CSP may interfere with this test. See
// https://crbug.com/415845.
testRunner.setIsolatedWorldContentSecurityPolicy(1, '');

function test() {
    function injectInlineScript(isolated) {
        var script = document.createElement('script');
        script.innerText = "console.log('EXECUTED in " + (isolated ? "isolated world" : "main world") + ".');";
        document.body.appendChild(script);
        window.postMessage("next", "*");
    }
    function injectInlineEventHandler(isolated) {
        var div = document.createElement('div');
        div.innerHTML = "<div onclick='function () {}'></div>";
        document.body.appendChild(div);
        window.postMessage("next", "*");
    }

    switch (tests) {
        case 4:
            console.log("Injecting in main world: this should fail.");
            injectInlineScript(false);
            injectInlineEventHandler(false);
            break;
        case 3:
            console.log("Injecting into isolated world without bypass: this should fail.");
            testRunner.evaluateScriptInIsolatedWorld(1, String(eval("injectInlineScript")) + "\ninjectInlineScript(true);");
            testRunner.evaluateScriptInIsolatedWorld(1, String(eval("injectInlineEventHandler")) + "\injectInlineEventHandler(true);");
            break;
        case 2:
            console.log("Starting to bypass main world's CSP: this should pass!");
            testRunner.setIsolatedWorldContentSecurityPolicy(1, 'script-src \'unsafe-inline\' *');
            testRunner.evaluateScriptInIsolatedWorld(1, String(eval("injectInlineScript")) + "\ninjectInlineScript(true);");
            testRunner.evaluateScriptInIsolatedWorld(1, String(eval("injectInlineEventHandler")) + "\injectInlineEventHandler(true);");
            break;
        case 1:
            console.log("Injecting into main world again: this should fail.");
            injectInlineScript(false);
            injectInlineEventHandler(false);
            break;
        case 0:
            testRunner.setIsolatedWorldContentSecurityPolicy(1, '');
            testRunner.notifyDone();
            break;
    }
}

document.addEventListener('DOMContentLoaded', test);
