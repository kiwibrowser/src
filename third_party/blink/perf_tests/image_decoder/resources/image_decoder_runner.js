function runImageDecoderPerfTests(imageFile, testDescription) {
  var isDone = false;

  function runTest() {
    var image = new Image();

    // When all the data is available...
    image.onload = function() {
      PerfTestRunner.addRunTestStartMarker();
      var startTime = PerfTestRunner.now();

      // Issue a decode command
      image.decode().then(function () {
        PerfTestRunner.measureValueAsync(PerfTestRunner.now() - startTime);
        PerfTestRunner.addRunTestEndMarker();

        // addRunTestEndMarker sets isDone to true once all iterations are
        // performed.
        if (!isDone) {
          runTest();
        }
      });
    }

    // Begin fetching the data
    image.src = imageFile + "?" + Math.random();
  }

  window.onload = function () {
    PerfTestRunner.startMeasureValuesAsync({
      unit: "ms",
      done: function () {
        isDone = true;
      },
      run: function () {
        runTest();
      },
      iterationCount: 20,
      description: testDescription,
      tracingCategories: 'blink',
      traceEventsToMeasure: ['ImageFrameGenerator::decode'],
    });
  };
}
