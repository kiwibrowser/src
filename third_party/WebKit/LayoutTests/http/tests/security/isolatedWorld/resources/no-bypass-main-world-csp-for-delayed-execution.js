if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
}

tests = 3;
window.addEventListener("message", function(event) {
    if (event.data == "next")
        tests -= 1;
    test(event.data);
}, false);

function test(message) {
    function injectInlineScript(script) {
        var scriptTag = document.createElement('script');
        scriptTag.innerText = script;
        document.body.appendChild(scriptTag);
        window.postMessage("next", "*");
    }

    function injectButtonWithInlineClickHandler(script) {
        var divTag = document.createElement('div');
        divTag.innerHTML = "<button id='button' onclick='" + script + "'></button>";
        document.body.appendChild(divTag);
        window.postMessage("done", "*");
    }

    var permissiveCSP = "script-src: * 'unsafe-eval' 'unsafe-inline'";

    switch (tests) {
        case 3:
            testRunner.setIsolatedWorldContentSecurityPolicy(1, permissiveCSP);
            testRunner.evaluateScriptInIsolatedWorld(1, String(injectInlineScript) + "\ninjectInlineScript('try { alert(\"PASS: Case " + tests + " was not blocked by a CSP.\"); } catch (e) { alert(\"FAIL: Case " + tests + " should not be blocked by a CSP.\"); }');");
            break;
        case 2:
            testRunner.setIsolatedWorldContentSecurityPolicy(1, permissiveCSP);
            testRunner.evaluateScriptInIsolatedWorld(1, String(injectInlineScript) + "\ninjectInlineScript('try { eval(\"alert(\\\'FAIL: Case " + tests + " should have been blocked by a CSP.\\\');\"); } catch( e) { console.log(e); alert(\\\'PASS: Case " + tests + " was blocked by a CSP.\\\'); }');");
            break;
        case 1:
            if (message != "done") {
                 testRunner.setIsolatedWorldContentSecurityPolicy(1, permissiveCSP);
                document.clickMessage = "PASS: Case " + tests + " was blocked by a CSP.";
                testRunner.evaluateScriptInIsolatedWorld(1, String(injectButtonWithInlineClickHandler) + "\ninjectButtonWithInlineClickHandler('document.clickMessage =\"FAIL: Case " + tests + " was not blocked by a CSP.\"');");
            } else {
                document.getElementById("button").click();
                alert(document.clickMessage);
                window.postMessage("next", "*");
            }
            break;
        case 0:
            testRunner.setIsolatedWorldContentSecurityPolicy(1, '');
            testRunner.notifyDone();
            break;
    }
}

document.addEventListener('DOMContentLoaded', test);
