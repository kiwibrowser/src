importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  const targetDocument = document;
  const targetSelector = '#div1'
  return new Promise(function(resolve, reject) {
    if (window.chrome && chrome.gpuBenchmarking) {
      scrollPageIfNeeded(targetSelector, targetDocument);
      var target = targetDocument.querySelector(targetSelector);
      var targetRect = target.getBoundingClientRect();
      var xPosition = targetRect.left + boundaryOffset;
      var yPosition = targetRect.top + boundaryOffset;

      chrome.gpuBenchmarking.pointerActionSequence( [
        {source: 'mouse',
         actions: [
            {name: 'pointerMove', x: xPosition, y: yPosition},
            {name: 'pointerDown', x: xPosition, y: yPosition, button: 'left'},
            {name: 'pointerMove', x: xPosition + 30, y: yPosition + 30},
            {name: 'pointerMove', x: xPosition + 30, y: yPosition},
            {name: 'pointerDown', x: xPosition + 30, y: yPosition, button: 'right'},
            {name: 'pointerMove', x: xPosition + 60, y: yPosition + 30},
            {name: 'pointerMove', x: xPosition + 30, y: yPosition + 20},
        ]}], resolve);
    } else {
      reject();
    }
  });
}
