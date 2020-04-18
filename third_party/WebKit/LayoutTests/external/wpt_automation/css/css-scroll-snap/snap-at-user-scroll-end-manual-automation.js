importAutomationScript('/pointerevents/pointerevent_common_input.js');

const WHEEL_SOURCE_TYPE = 2;

function smoothScroll(pixels_to_scroll, start_x, start_y,
    direction, speed_in_pixels_s) {
  return new Promise((resolve, reject) => {
    chrome.gpuBenchmarking.smoothScrollBy(pixels_to_scroll, resolve, start_x,
        start_y, WHEEL_SOURCE_TYPE, direction, speed_in_pixels_s);
  });
}

function waitForAnimationEnd() {
  const MAX_FRAME = 500;
  var last_changed_frame = 0;
  var last_window_x = window.scrollX;
  var last_window_y = window.scrollY;
  return new Promise((resolve, reject) => {
    function tick(frames) {
      // We requestAnimationFrame either for 500 frames or until 5 frames with
      // no change have been observed.
      if (frames >= MAX_FRAME || frames - last_changed_frame > 5) {
        resolve();
      } else {
        if (window.scrollX != last_window_x ||
            window.scrollY != last_window_y) {
          last_changed_frame = frames;
          last_window_x = window.scrollX;
          last_window_y = window.scrollY;
        }
        requestAnimationFrame(tick.bind(this, frames + 1));
      }
    }
    tick(0);
  });
}

function inject_input() {
  return smoothScroll(2, 20, 20, 'downright', 4000).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btn');
  });
}