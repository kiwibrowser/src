(function() {
  "use strict";
  // Define functions one by one and do not override the whole
  // test_driver_internal as it masks the new testing fucntions
  // that will be added in the future.

  window.test_driver_internal.click = function(element, coords) {
    return new Promise(function(resolve, reject) {
      if (window.chrome && chrome.gpuBenchmarking) {
        chrome.gpuBenchmarking.pointerActionSequence(
            [{
              source: 'mouse',
              actions: [
              {name: 'pointerMove', x: coords.x, y: coords.y},
              {name: 'pointerDown', x: coords.x, y: coords.y, button: 'left'},
              {name: 'pointerUp', button: 'left'}
              ]
            }],
            resolve);
      } else {
        reject(new Error("GPU benchmarnking is not enabled."));
      }
    });
  };

})();
