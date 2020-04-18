function smoothScroll(pixels_to_scroll, start_x, start_y, gesture_source_type, direction, speed_in_pixels_s) {
  return new Promise((resolve, reject) => {
    if (chrome && chrome.gpuBenchmarking) {
      chrome.gpuBenchmarking.smoothScrollBy(pixels_to_scroll,
                                            resolve,
                                            start_x,
                                            start_y,
                                            gesture_source_type,
                                            direction,
                                            speed_in_pixels_s);
    } else {
      reject('This test requires chrome.gpuBenchmarking');
    }
  });
}

function pinchBy(scale, centerX, centerY, speed_in_pixels_s, gesture_source_type) {
  return new Promise((resolve, reject) => {
    if (chrome && chrome.gpuBenchmarking) {
      chrome.gpuBenchmarking.pinchBy(scale,
                                     centerX,
                                     centerY,
                                     resolve,
                                     speed_in_pixels_s,
                                     gesture_source_type);
    } else {
      reject('This test requires chrome.gpuBenchmarking');
    }
  });
}


function mouseMoveTo(xPosition, yPosition) {
  return new Promise(function(resolve, reject) {
    if (chrome && chrome.gpuBenchmarking) {
      chrome.gpuBenchmarking.pointerActionSequence([
        {source: 'mouse',
         actions: [
            { name: 'pointerMove', x: xPosition, y: yPosition },
      ]}], resolve);
    } else {
      reject('This test requires chrome.gpuBenchmarking');
    }
  });
}