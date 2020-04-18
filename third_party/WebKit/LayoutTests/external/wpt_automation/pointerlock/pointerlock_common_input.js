// This file contains the commonly used functions in pointerlock tests.

const boundaryOffset = 2;

function scrollPageIfNeeded(targetSelector, targetDocument) {
  const target = targetDocument.querySelector(targetSelector);
  const targetRect = target.getBoundingClientRect();
  if (targetRect.top < 0 || targetRect.left < 0 || targetRect.bottom > window.innerHeight || targetRect.right > window.innerWidth)
    window.scrollTo(targetRect.left, targetRect.top);
}

function mouseClickInTarget(targetSelector, targetFrame, button) {
  let targetDocument = document;
  let frameLeft = 0;
  let frameTop = 0;
  if (button === undefined) {
    button = 'left';
  }
  if (targetFrame !== undefined) {
    targetDocument = targetFrame.contentDocument;
    const frameRect = targetFrame.getBoundingClientRect();
    frameLeft = frameRect.left;
    frameTop = frameRect.top;
  }
  return new Promise(function(resolve, reject) {
    if (window.chrome && chrome.gpuBenchmarking) {
      scrollPageIfNeeded(targetSelector, targetDocument);
      const target = targetDocument.querySelector(targetSelector);
      const targetRect = target.getBoundingClientRect();
      const xPosition = frameLeft + targetRect.left + boundaryOffset;
      const yPosition = frameTop + targetRect.top + boundaryOffset;
      chrome.gpuBenchmarking.pointerActionSequence(
          [{
            source: 'mouse',
            actions: [
              {name: 'pointerMove', x: xPosition, y: yPosition},
              {name: 'pointerDown', x: xPosition, y: yPosition, button: button},
              {name: 'pointerUp', button: button}
            ]
          }],
          resolve);
    } else {
      reject();
    }
  });
}

{
  const pointerlock_automation = async_test("PointerLock Automation");
  // Defined in every test and should return a promise that gets resolved when input is finished.
  inject_input().then(function() {
    pointerlock_automation.done();
  });
}
