importScripts('../../resources/testharness.js');

promise_test(function(test) {
    var durationMsec = 100;

    return new Promise(function(resolve) {
        performance.mark('startMark');
        setTimeout(resolve, durationMsec);
      }).then(function() {
          performance.mark('endMark');
          performance.measure('measure', 'startMark', 'endMark');

          var startMark = performance.getEntriesByName('startMark')[0];
          var endMark = performance.getEntriesByName('endMark')[0];
          var measure = performance.getEntriesByType('measure')[0];

          assert_equals(measure.startTime, startMark.startTime);
          assert_approx_equals(endMark.startTime - startMark.startTime,
                               measure.duration, 0.001);
          assert_greater_than(measure.duration, durationMsec);

          assert_equals(performance.getEntriesByType('mark').length, 2);
          assert_equals(performance.getEntriesByType('measure').length, 1);
          performance.clearMarks('startMark');
          performance.clearMeasures('measure');
          assert_equals(performance.getEntriesByType('mark').length, 1);
          assert_equals(performance.getEntriesByType('measure').length, 0);
      });
  }, 'User Timing');

promise_test(function(test) {
    return fetch('../../resources/dummy.txt')
      .then(function(resp) {
          return resp.text();
        })
      .then(function(t) {
          // TODO(hiroshige): The resource timing entry for dummy.txt is added
          // about at the same time as, but not necessarily before,
          // |resp.text()| is resolved. https://crbug.com/507169
          // We add setTimeout() here as temporary fix.
          return new Promise(function(resolve, reject) {
              setTimeout(function() { resolve(t); }, 100);
            });
        })
      .then(function(t) {
          var expectedResources = ['/resources/testharness.js', '/resources/dummy.txt'];
          assert_equals(performance.getEntriesByType('resource').length, expectedResources.length);
          for (var i = 0; i < expectedResources.length; i++) {
              var entry = performance.getEntriesByType('resource')[i];
              assert_true(entry.name.endsWith(expectedResources[i]));
              assert_equals(entry.workerStart, 0);
              assert_greater_than(entry.startTime, 0);
              assert_greater_than(entry.responseEnd, entry.startTime);
          }
          return new Promise(function(resolve) {
              performance.onresourcetimingbufferfull = resolve;
              performance.setResourceTimingBufferSize(expectedResources.length);
            });
        })
      .then(function() {
          performance.clearResourceTimings();
          assert_equals(performance.getEntriesByType('resource').length, 0);
        })
  }, 'Resource Timing');

done();
