// This test code is shared between resource-timing-sizes-cache.html and
// resource-timing-sizes-cache-worker.html

if (typeof document === 'undefined') {
    importScripts('/resources/testharness.js',
                  '/misc/resources/run-async-tasks-promise.js');
}

// The header bytes are expected to be > |minHeaderSize| and
// < |maxHeaderSize|. If they are outside this range the test will fail.
const minHeaderSize = 100;
const maxHeaderSize = 1024;

// The size of this resource must be > maxHeaderSize for the test to be
// reliable.
var url = new URL('/resources/square.png', location.href).href;
const expectedSize = 18299;

function checkBodySizeFields(entry, expectedSize) {
    assert_equals(entry.decodedBodySize, expectedSize, 'decodedBodySize');
    assert_equals(entry.encodedBodySize, expectedSize, 'encodedBodySize');
}

function checkResourceSizes() {
    var entries = performance.getEntriesByName(url);
    assert_equals(entries.length, 3, 'Wrong number of entries');
    var seenCount = 0;
    for (var entry of entries) {
        checkBodySizeFields(entry, expectedSize);
        if (seenCount === 0) {
            // 200 response
            assert_between_exclusive(entry.transferSize,
                                     expectedSize + minHeaderSize,
                                     expectedSize + maxHeaderSize,
                                     '200 transferSize');
        } else if (seenCount === 1) {
            // from cache
            assert_equals(entry.transferSize, 0, 'cached transferSize');
        } else if (seenCount === 2) {
            // 304 response
            assert_between_exclusive(entry.transferSize, minHeaderSize,
                                     maxHeaderSize, '304 transferSize');
        } else {
            assert_unreached('Too many matching entries');
        }
        ++seenCount;
    }
}

promise_test(() => {
    // Use a different URL every time so that the cache behaviour does not
    // depend on execution order.
    url = url + '?unique=' + Math.random().toString().substring(2);
    var eatBody = response => response.arrayBuffer();
    var mustRevalidate = {headers: {'Cache-Control': 'max-age=0'}};
    return fetch(url)
        .then(eatBody)
        .then(() => fetch(url))
        .then(eatBody)
        .then(() => fetch(url, mustRevalidate))
        .then(eatBody)
        .then(runAsyncTasks)
        .then(checkResourceSizes);
}, 'PerformanceResourceTiming sizes caching test');

done();
