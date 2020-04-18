importAutomationScript('/pointerlock/pointerlock_common_input.js');

function sendMouseClick(x, y) {
    return new Promise(function(resolve, reject) {
        if (window.chrome && chrome.gpuBenchmarking) {
          var pointerActions =
              [{source: "mouse",
                actions: [
                  { name: "pointerDown", x: x, y: y },
                  { name: "pointerUp" },
              ]}];
          chrome.gpuBenchmarking.pointerActionSequence(pointerActions, resolve);
        }
        else {
            reject();
        }
    });
}

function moveMouseTo(x, y) {
    return new Promise(function(resolve, reject) {
        if (window.chrome && chrome.gpuBenchmarking) {
          var pointerActions =
              [{source: "mouse",
                actions: [
                  { name: "pointerMove", x: x, y: y },
              ]}];
          chrome.gpuBenchmarking.pointerActionSequence(pointerActions, resolve);
        }
        else {
            reject();
        }
    });
}

function inject_input() {
  a = 5.2; b = 4.8;
  return sendMouseClick(100,100).then(function() {
    return moveMouseTo(100, 100);
  }).then(function() {
    return moveMouseTo(100 + a, 100 + b);
  }).then(function() {
    return moveMouseTo(100 + a + b, 100 + a + b);
  }).then(function() {
    return moveMouseTo(100 + b, 100 + a);
  }).then(function() {
    return moveMouseTo(100, 100);
  }).then(function() {
    return sendMouseClick(100, 100);
  });
}
