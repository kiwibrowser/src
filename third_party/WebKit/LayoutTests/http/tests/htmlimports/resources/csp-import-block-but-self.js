function test(actual, expected, message)
{
    if (actual === expected)
        console.log("PASS");
    else
        console.log("FAIL:" + message);
}

if (window.testRunner)
    testRunner.dumpAsText();

test(document.inlineScriptHasRun, undefined, "document.inlineScriptHasRun");
test(document.externalScriptHasRun, true, "document.externalScriptHasRun");
test(document.corsExternalScriptHasRun, undefined, "document.corsExternalScriptHasRun");
test(document.evalFromInlineHasRun, undefined, "document.evalFromInlineHasRun");
test(document.evalFromExternalHasRun, undefined, "document.evalFromExternalHasRun");
test(document.evalFromCorsExternalHasRun, undefined, "document.evalFromCorsExternalHasRun");

