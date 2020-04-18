function log(text)
{
    var pre = document.createElement('pre');
    pre.textContent = text;
    document.body.appendChild(pre);
}

function shouldBe(a, b)
{
    var evalA, evalB;
    try {
        evalA = eval(a);
    } catch(e) {
        evalA = e.toString();
    }

    try {
        evalB = eval(b);
    } catch(e) {
        evalB = e.toString();
    }

    var message;
    if (evalA === evalB) {
        message = "PASS: " + a + " should be '" + evalB + "' and is.";
    } else {
       message = "FAIL: " + a + " should be '" + evalB + "' but instead is " + evalA + ".";
    }

    message = String(message).replace(/\n/g, "");
    log(message);
}

if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
    window.onload = function() {
        runTest();
        testRunner.notifyDone();
    }
}
