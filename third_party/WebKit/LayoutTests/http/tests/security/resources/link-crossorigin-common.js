// Tiny test rig for all security/link-crossorigin-*.html tests,
// which exercise <link> + CORS variations.

window.jsTestIsAsync = true;

if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
}

// The common case is to have four sub-tests. To override
// for a test, assign window.testCount.
var defaultTestCount = 4;

var eventCount = 0;
testCount = window.testCount || defaultTestCount;

function finish(pass, msg, event) {
    if (pass)
        testPassed(msg || "");
    else
        testFailed(msg || "");

    // Verify that the stylesheet is (in)accessible.
    // (only report failures so as to avoid subtest ordering instability.)
    if (event !== undefined && event.target && event.target.rel === "stylesheet") {
        for (var i = 0; i < document.styleSheets.length; ++i) {
            if (document.styleSheets[i].href !== event.target.href)
                continue;
            if (event.type === "load") {
                try {
                    if (document.styleSheets[i].cssRules[0].cssText.length >= 0)
                       ;
                    if (document.styleSheets[i].cssRules[0].cssText.indexOf("green") == -1)
                        testFailed("Failed to find occurrence of 'green' in stylesheet: " + event.target.href);
                } catch (e) {
                    testFailed("Failed to access contents of stylesheet: " + event.target.href);
                }
            } else {
                try {
                    // Will throw as .cssRules should return 0.
                    if (document.styleSheets[i].cssRules[0].cssRules[0])
                        ;
                    testFailed("Should not be able to access contents of stylesheet: " + event.target.href);
                } catch (e) {
                    ;
                }
            }
            break;
        }
    }
    if (++eventCount >= testCount)
        finishJSTest();
}

function pass(event) { finish(true, "", event); }
function fail(event) { finish(false, "", event); }
