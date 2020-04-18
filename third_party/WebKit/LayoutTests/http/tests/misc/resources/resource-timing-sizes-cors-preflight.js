// This test code is shared between resource-timing-sizes-cors-preflight.html
// and resource-timing-sizes-cors-preflight-worker.html

if (typeof document === 'undefined') {
    importScripts('/resources/testharness.js',
                  '/resources/get-host-info.js?pipe=sub',
                  '/misc/resources/run-async-tasks-promise.js');
}

// Because apache decrements the Keep-Alive max value on each request, the
// transferSize will vary slightly between requests for the same resource.
const fuzzFactor = 3;  // bytes

const hostInfo = get_host_info();
const url = new URL('/misc/resources/cors-preflight.php',
                    hostInfo['HTTP_REMOTE_ORIGIN']).href;

// The header bytes are expected to be > |minHeaderSize| and
// < |maxHeaderSize|. If they are outside this range the test will fail.
const minHeaderSize = 100;
const maxHeaderSize = 1024;

function checkResourceSizes() {
    var lowerBound, upperBound;
    var entries = performance.getEntriesByName(url);
    assert_equals(entries.length, 3, 'Wrong number of entries');
    // Firefox 47 puts the preflight after the request in the timeline.
    // Sort by requestStart for compatibility.
    entries.sort((a, b) => b.requestStart - a.requestStart);
    var seenCount = 0;
    for (var entry of entries) {
        switch (seenCount) {
        case 0:
            assert_greater_than(entry.transferSize, 0,
                                'no preflight transferSize');
            lowerBound = entry.transferSize - fuzzFactor;
            upperBound = entry.transferSize + fuzzFactor;
            break;

        case 1:
            assert_between_exclusive(entry.transferSize, minHeaderSize,
                                     maxHeaderSize,
                                     'preflight transferSize');
            break;

        case 2:
            assert_between_exclusive(entry.transferSize, lowerBound,
                                     upperBound,
                                     'preflighted transferSize');
            break;
        }
        ++seenCount;
    }
}

promise_test(() => {
    var eatBody = response => response.arrayBuffer();
    var requirePreflight = {headers: {'X-Require-Preflight': '1'}};
    return fetch(url)
        .then(eatBody)
        .then(() => fetch(url, requirePreflight))
        .then(eatBody)
        .then(runAsyncTasks)
        .then(checkResourceSizes);
}, 'PerformanceResourceTiming sizes Fetch with preflight test');

done();
