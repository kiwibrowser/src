if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
}

tests = 4;
window.addEventListener("message", function(message) {
    tests -= 1;
    test();
}, false);

function test() {
    function injectInlineStyle(shouldSucceed, tests) {
        var id = 'inline' + tests;
        var div = document.createElement('div');
        div.id = id;
        document.body.appendChild(div);
        var style = document.createElement('style');
        style.innerText = '#' + id + ' { color: red; }';
        document.body.appendChild(style);
        var success = window.getComputedStyle(document.getElementById(id)).color === "rgb(255, 0, 0)";
        if (shouldSucceed) {
            if (success)
                console.log("PASS: Style assignment in test " + tests + " was blocked by CSP.");
            else
                console.log("FAIL: Style assignment in test " + tests + " was not blocked by CSP.");
        } else {
            if (success)
                console.log("FAIL: Style assignment in test " + tests + " was blocked by CSP.");
            else
                console.log("PASS: Style assignment in test " + tests + " was not blocked by CSP.");
        }
        window.postMessage("next", "*");
    }
    function injectInlineStyleAttribute(shouldSucceed, tests) {
        var id = 'attribute' + tests;
        var div = document.createElement('div');
        div.id = id;
        document.body.appendChild(div);
        div.setAttribute('style', 'color: red;');
        var success = window.getComputedStyle(document.getElementById(id)).color === "rgb(255, 0, 0)";
        if (shouldSucceed) {
            if (success)
                console.log("PASS: Style attribute assignment in test " + tests + " was blocked by CSP.");
            else
                console.log("FAIL: Style attribute assignment in test " + tests + " was not blocked by CSP.");
        } else {
            if (success)
                console.log("FAIL: Style attribute assignment in test " + tests + " was blocked by CSP.");
            else
                console.log("PASS: Style attribute assignment in test " + tests + " was not blocked by CSP.");
        }
        window.postMessage("next", "*");
    }

    switch (tests) {
        case 4:
            console.log("Injecting in main world: this should fail.");
            injectInlineStyle(false, tests);
            break;
        case 3:
            console.log("Injecting into isolated world without bypass: this should fail.");
            testRunner.evaluateScriptInIsolatedWorld(1, String(eval("injectInlineStyle")) + "\ninjectInlineStyle(false," + tests + ");");
            testRunner.evaluateScriptInIsolatedWorld(1, String(eval("injectInlineStyleAttribute")) + "\ninjectInlineStyleAttribute(false," + tests + ");");
            break;
        case 2:
            console.log("Starting to bypass main world's CSP: this should pass!");
            testRunner.setIsolatedWorldContentSecurityPolicy(1, 'style-src \'unsafe-inline\' *');
            testRunner.evaluateScriptInIsolatedWorld(1, String(eval("injectInlineStyle")) + "\ninjectInlineStyle(true," + tests + ");");
            testRunner.evaluateScriptInIsolatedWorld(1, String(eval("injectInlineStyleAttribute")) + "\ninjectInlineStyleAttribute(true," + tests + ");");
            break;
        case 1:
            console.log("Injecting into main world again: this should fail.");
            injectInlineStyle(false, tests);
            break;
        case 0:
            testRunner.setIsolatedWorldContentSecurityPolicy(1, '');
            testRunner.notifyDone();
            break;
    }
}

document.addEventListener('DOMContentLoaded', test);
