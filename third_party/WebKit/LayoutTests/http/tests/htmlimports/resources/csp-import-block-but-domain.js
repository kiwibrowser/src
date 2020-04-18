if (window.testRunner)
    testRunner.dumpAsText();

function test(actual, expected, message)
{
    if (actual === expected)
        console.log("PASS");
    else
        console.log("FAIL:" + message);
}

test(document.inlineScriptHasRun, undefined, "document.inlineScriptHasRun");
test(document.externalScriptHasRun, undefined, "document.externalScriptHasRun");
test(document.corsExternalScriptHasRun, true, "document.corsExternalScriptHasRun");
test(document.evalFromInlineHasRun, undefined, "document.evalFromInlineHasRun");
test(document.evalFromExternalHasRun, undefined, "document.evalFromExternalHasRun");
test(document.evalFromCorsExternalHasRun, undefined, "document.evalFromCorsExternalHasRun");

