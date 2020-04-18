importAutomationScript('/pointerevents/pointerevent_common_input.js');
const scale = 5;
const width = 3;
const height = 3;
function inject_input() {
  return testInputType("mouse").then(function() {
    return testInputType("touch");
  }).then(function() {
    return testInputType("pen");
  });
}

function testInputType(inputSource) {
  var targetFrame = document.querySelector('#innerFrame');
  var frameRect = targetFrame.getBoundingClientRect();
  frameLeft = frameRect.left;
  frameTop = frameRect.top;

  target = [{x: 10, y: 10}, {x: 30, y: 50}, {x: 50, y: 30}]
  xPosition = []
  yPosition = []
  for (var i = 0; i < target.length; i++) {
    xPosition.push((target[i].x + width / 2.0) * scale + frameLeft);
    yPosition.push((target[i].y + height / 2.0) * scale + frameTop);
  }
  return sendInputAt(inputSource, xPosition[0], yPosition[0]).then(function() {
    return sendInputAt(inputSource, xPosition[1], yPosition[1]);
  }).then(function() {
    return sendInputAt(inputSource, xPosition[2], yPosition[2]);
  });

}

function sendInputAt(inputSource, xPosition, yPosition) {
  if (inputSource == "touch") {
    pointerActions = [{name: 'pointerDown', x: xPosition, y: yPosition},
                      {name: 'pointerMove', x: xPosition + 1, y: yPosition + 1},
                      {name: 'pointerUp'}]
  }
  else {
    pointerActions = [{name: 'pointerMove', x: xPosition, y: yPosition},
                      {name: 'pointerDown', x: xPosition, y: yPosition},
                      {name: 'pointerUp'}]
  }

  return new Promise(function(resolve, reject) {
    if (window.chrome && chrome.gpuBenchmarking) {
      chrome.gpuBenchmarking.pointerActionSequence(
          [{
            source: inputSource,
            actions: pointerActions
          }],
          resolve);
    } else {
      reject();
    }
  });
}
