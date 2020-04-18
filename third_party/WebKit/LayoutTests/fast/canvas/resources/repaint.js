if (window.testRunner)
    testRunner.waitUntilDone();

function runRepaintTest()
{
    window.requestAnimationFrame(function() {
        window.setTimeout(function() {
            repaintTest();
            if (window.testRunner)
                testRunner.notifyDone();
        }, 0);
    });
}
