// Given a piece of 'code', runs it in the worklet, then once loaded waits for
// layout and paint, before finishing the test.
//
// Usage:
//   importPaintWorkletAndTerminateTestAfterAsyncPaint('/* worklet code goes here. */');

function importPaintWorkletAndTerminateTestAfterAsyncPaint(code) {
    if (window.testRunner) {
      testRunner.waitUntilDone();
    }

    var blob = new Blob([code], {type: 'text/javascript'});
    CSS.paintWorklet.addModule(URL.createObjectURL(blob)).then(function() {
        runAfterLayoutAndPaint(function() {
            if (window.testRunner) {
                testRunner.notifyDone();
            }
        });
    });
}
