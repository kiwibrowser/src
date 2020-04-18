
if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
    waitUntilWorkerThreadsExit(runTests);
} else {
    log("NOTE: This test relies on functionality in DumpRenderTree to detect when workers have exited - test results will be incorrect when run in a browser.");
    runTests();
}

function runTests()
{
    Promise.all([
        createWorkerFrame("frame1", "worker1"),
        createWorkerFrame("frame2", "worker1,worker2"),
        createWorkerFrame("frame3", "worker3"),
        createWorkerFrame("frame4", "worker1")])
    .then(function() {
        waitUntilThreadCountMatches(closeFrame1, 3);
    });
}

function createWorkerFrame(id, workerNames)
{
    return new Promise(function(resolve) {
            var frame = document.createElement("iframe");
            frame.onload = function() { resolve(frame); };
            frame.setAttribute("id", id);
            frame.setAttribute("src", "resources/create-shared-worker-frame.html?" + workerNames);
            document.body.appendChild(frame);
        })
        .then(function(frame) {
            return Promise.all(frame.contentWindow.wait_pong_promises);
        });
}

function closeFrame(id)
{
    var frame = document.getElementById(id);
    frame.parentNode.removeChild(frame);
}

function reloadWorkerFrame(id)
{
    return new Promise(function(resolve) {
            var frame = document.getElementById(id);
            frame.onload = function() { resolve(frame); };
            frame.contentWindow.location.reload();
        })
        .then(function(frame) {
            return Promise.all(frame.contentWindow.wait_pong_promises);
        });
}

function closeFrame1()
{
    closeFrame("frame1");
    ensureThreadCountMatches(closeFrame2, 3);
}

function closeFrame2()
{
    log("PASS: Frame1 closed, shared workers kept running");
    closeFrame("frame2");
    ensureThreadCountMatches(closeFrame3, 2);
}

function closeFrame3()
{
    log("PASS: Frame2 closed, shared worker2 exited");
    closeFrame("frame3");
    ensureThreadCountMatches(reloadFrame4, 1);
}

function reloadFrame4() {
    log("PASS: Frame3 closed, shared worker3 exited");
    reloadWorkerFrame("frame4")
        .then(function() {
            ensureThreadCountMatches(closeFrame4, 1);
        });
}

function closeFrame4()
{
    log("PASS: Frame4 reloaded");
    closeFrame("frame4");
    waitUntilWorkerThreadsExit(complete);
}

function complete()
{
    log("PASS: Frame4 closed, all workers closed");
    log("DONE");
    if (window.testRunner)
        testRunner.notifyDone();
}
