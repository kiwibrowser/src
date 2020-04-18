function workerReloadRepeated(workerJs) {
    if (window.testRunner) {
        testRunner.dumpAsText();
        testRunner.waitUntilDone();
    }

    var repeat = 0;
    var regex = new RegExp("#repeat(\\d+)");
    var match = regex.exec(location.hash);
    if (match)
        repeat = parseInt(match[1]);

    if (repeat === 10) {
        document.getElementById("result").innerHTML = "Done.";
        if (window.testRunner)
            testRunner.notifyDone();
    } else {
        new Worker("resources/worker-open-reload.js").onmessage = function (evt) {
            location.hash = "repeat" + (repeat + 1);
            location.reload()
        };
    }
}
