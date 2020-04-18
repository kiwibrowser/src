importScripts('../../../resources/js-test.js');

self.jsTestIsAsync = true;

description('Test createImageBitmap with invalid blobs in workers.');

self.addEventListener('message', function(e) {
  createImageBitmap(e.data).then(function() {
    testFailed('Promise fulfuilled.');
    finishJSTest();
  }, function(ex) {
    testPassed('Promise rejected: ' + ex);
    finishJSTest();
  });
});
