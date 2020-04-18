importScripts('../../../resources/js-test.js');

self.jsTestIsAsync = true;

description('Test createImageBitmap with data in workers.');

var bitmapWidth;
var bitmapHeight;

self.addEventListener('message', function(e) {
  createImageBitmap(e.data).then(function(imageBitmap) {
    testPassed('Promise fulfilled.');
    bitmapWidth = imageBitmap.width;
    bitmapHeight = imageBitmap.height;
    shouldBe("bitmapWidth", "50");
    shouldBe("bitmapHeight", "50");
    finishJSTest();
  }, function() {
    testFailed('Promise rejected.');
    finishJSTest();
  });
});
