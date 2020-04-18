<!DOCTYPE html>
<title>Block reading offscreen canvas in worker via StrictCanvasTainting setting</title>
<script src="/resources/testharness.js"></script>
<script src="/resources/testharnessreport.js"></script>
<script id='myWorker' type='text/worker'>
self.onmessage = function(e) {
    var offCanvas = new OffscreenCanvas(100, 100);
    var context = offCanvas.getContext('2d');

    var name1;
    try {
       var imageData = context.getImageData(0, 0, 100, 100);
       name1 = "NoError";
    } catch (ex) {
       name1 = ex.name;
    }

    var name2;
    try {
         var imageCanvas = new OffscreenCanvas(100, 100);
         var imageCtx = imageCanvas.getContext('2d');
         imageCtx.fillStyle = "blue";
         imageCtx.fillRect(0, 0, 100, 100);
         var imageBitMap = imageCanvas.transferToImageBitmap();

         context.drawImage(imageBitMap, 0, 0, 100, 100);
         var imageData = context.getImageData(0, 0, 100, 100);
         name2 = "NoError";
    } catch (ex) {
         name2 = ex.name;
    }
    self.postMessage([name1, name2]);
};
</script> 
<script>
if (window.testRunner) {
    testRunner.overridePreference("WebKitDisableReadingFromCanvas", true);
}

async_test(function() {
    var blob = new Blob([document.getElementById('myWorker').textContent]);
    var worker = new Worker(URL.createObjectURL(blob));
    worker.onmessage = this.step_func(function (e) {
       assert_equals(e.data[0], "SecurityError", "First test on getImageData");
       assert_equals(e.data[1], "SecurityError", "Second test on getImageData");
       this.done();
    });

    worker.postMessage("");
}, "getImageData should throw on worker");


</script>
