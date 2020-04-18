importScripts('../../../resources/js-test.js');

self.jsTestIsAsync = true;

description('Test createImageBitmap with invalid arguments in workers.');

var reason;

function shouldBeRejected(promise, message) {
    return promise.then(function() {
        testFailed('Resolved unexpectedly: ' + message);
    }, function(e) {
        reason = e;
        testPassed('Rejected as expected: ' + message);
        shouldBeTrue('reason instanceof Error');
        debug(String(e));
    });
}

var data;

self.addEventListener('message', function(e) {
    data = e.data;
    Promise.resolve().then(function() {
        return shouldBeRejected(createImageBitmap(null, 0, 0, 10, 10), 'null');
    }).then(function() {
        return shouldBeRejected(createImageBitmap(data, 0, 0, 10, 0), 'invalid area');
    }).then(function() {
        return shouldBeRejected(createImageBitmap(data, 0, 0, 0, 10), 'invalid area');
    }).catch(function(e) {
        testFailed('Unexpected rejection: ' + e);
    }).then(finishJSTest, finishJSTest);
});
