// CanvasRunner is a wrapper of PerformanceTests/resources/runner.js for canvas tests.
(function () {
    var MEASURE_DRAW_TIMES = 50;
    var MAX_MEASURE_DRAW_TIMES = 1000;
    var MAX_MEASURE_TIME_PER_FRAME = 1000; // 1 sec
    var currentTest = null;
    var isTestDone = false;

    var CanvasRunner = {};

    CanvasRunner.start = function (test) {
        PerfTestRunner.startMeasureValuesAsync({
            unit: 'runs/s',
            description: test.description,
            done: testDone,
            run: function() {
                if (!test.doRun) {
                    CanvasRunner.logFatalError("doRun must be set.");
                    return;
                }
                currentTest = test;
                runTest();
            }});
    }

    function runTest() {
        try {
            if (currentTest.preRun)
                currentTest.preRun();

            var start = PerfTestRunner.now();
            var count = 0;
            while ((PerfTestRunner.now() - start <= MAX_MEASURE_TIME_PER_FRAME) && (count * MEASURE_DRAW_TIMES < MAX_MEASURE_DRAW_TIMES)) {
                for (var i = 0; i < MEASURE_DRAW_TIMES; i++) {
                    currentTest.doRun();
                }
                count++;
            }
            if (currentTest.ensureComplete)
                currentTest.ensureComplete();
            var elapsedTime = PerfTestRunner.now() - start;
            if (currentTest.postRun)
                currentTest.postRun();

            PerfTestRunner.measureValueAsync(MEASURE_DRAW_TIMES * count * 1000 / elapsedTime);
        } catch(err) {
            CanvasRunner.logFatalError("test fails due to GPU issue. " + err);
            return;
        }

        if (!isTestDone)
            requestAnimationFrame(runTest);
    }

    function testDone() {
        isTestDone = true;
    }

    CanvasRunner.logFatalError = function (text) {
        PerfTestRunner.logFatalError(text);
    }

    CanvasRunner.startPlayingAndWaitForVideo = function (video, callback) {
        var gotPlaying = false;
        var gotTimeUpdate = false;

        var maybeCallCallback = function() {
            if (gotPlaying && gotTimeUpdate && callback) {
                callback(video);
                callback = undefined;
                video.removeEventListener('playing', playingListener, true);
                video.removeEventListener('timeupdate', timeupdateListener, true);
            }
        };

        var playingListener = function() {
            gotPlaying = true;
            maybeCallCallback();
        };

        var timeupdateListener = function() {
            // Checking to make sure the current time has advanced beyond
            // the start time seems to be a reliable heuristic that the
            // video element has data that can be consumed.
            if (video.currentTime > 0.0) {
                gotTimeUpdate = true;
                maybeCallCallback();
            }
        };

        video.addEventListener('playing', playingListener, true);
        video.addEventListener('timeupdate', timeupdateListener, true);
        video.loop = true;
        video.play();
    }

    window.CanvasRunner = CanvasRunner;
})();
