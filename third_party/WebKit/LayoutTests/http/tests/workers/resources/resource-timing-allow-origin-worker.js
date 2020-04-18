importScripts('../../resources/testharness.js');

function fetchAndWaitForResourceTimingEntry(url) {
    return fetch(url)
      .then(function(resp) {
          return resp.text();
        })
      .then(function(t) {
          // TODO(hiroshige): The resource timing entry is added
          // about at the same time as, but not necessarily before,
          // |resp.text()| is resolved. https://crbug.com/507169
          // We add setTimeout() here as temporary fix.
          return new Promise(function(resolve, reject) {
              setTimeout(function() { resolve(t); }, 100);
            });
        })
      .then(function(t) {
          var entries = performance.getEntriesByName(url);
          if (entries.length == 0)
              throw "no performance entry for " + url;
          return entries[0];
        });
}

function assertTimingAllowed(entry) {
    assert_greater_than(entry.startTime, 0, 'startTime');
    assert_greater_than(entry.fetchStart, 0, 'fetchStart');
    assert_greater_than(entry.domainLookupStart, 0, 'domainLookupStart');
    assert_greater_than(entry.domainLookupEnd, 0, 'domainLookupEnd');
    assert_greater_than(entry.connectStart, 0, 'connectStart');
    assert_greater_than(entry.connectEnd, 0, 'connectEnd');
    assert_greater_than(entry.requestStart, 0, 'requestStart');
    assert_greater_than(entry.responseStart, 0, 'responseStart');
    assert_greater_than(entry.responseEnd, 0, 'responseEnd');
}

function assertTimingNotAllowed(entry) {
    assert_greater_than(entry.startTime, 0, 'startTime');
    assert_greater_than(entry.fetchStart, 0, 'fetchStart');
    assert_equals(entry.domainLookupStart, 0, 'domainLookupStart');
    assert_equals(entry.domainLookupEnd, 0, 'domainLookupEnd');
    assert_equals(entry.connectStart, 0, 'connectStart');
    assert_equals(entry.connectEnd, 0, 'connectEnd');
    assert_equals(entry.requestStart, 0, 'requestStart');
    assert_equals(entry.responseStart, 0, 'responseStart');
    assert_greater_than(entry.responseEnd, 0, 'responseEnd');
}

var url = 'http://localhost:8000/workers/resources/timing-allow-origin.php';

promise_test(function(test) {
    return fetchAndWaitForResourceTimingEntry(url)
        .then(assertTimingNotAllowed);
  }, 'No timing-allow-origin');

promise_test(function(test) {
    return fetchAndWaitForResourceTimingEntry(url + '?origin=*')
        .then(assertTimingAllowed);
  }, 'timing-allow-origin: *');

promise_test(function(test) {
    return fetchAndWaitForResourceTimingEntry(url + '?origin=http://127.0.0.1:8000')
        .then(assertTimingAllowed);
  }, 'timing-allow-origin: page origin');

promise_test(function(test) {
    return fetchAndWaitForResourceTimingEntry(url + '?origin=http://localhost:8000')
        .then(assertTimingNotAllowed);
  }, 'timing-allow-origin: other origin');

done();
