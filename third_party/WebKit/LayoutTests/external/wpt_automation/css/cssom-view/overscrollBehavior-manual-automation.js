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
  var last_root_x = root.scrollLeft;
  var last_root_y = root.scrollTop;
  var last_container_x = container.scrollLeft;
  var last_container_y = container.scrollTop;
  return new Promise((resolve, reject) => {
    function tick(frames) {
    // We requestAnimationFrame either for 500 frames or until 5 frames with
    // no change have been observed.
      if (frames >= MAX_FRAME || frames - last_changed_frame > 5) {
        resolve();
      } else {
        if (root.scrollLeft != last_root_x ||
            root.scrollTop != last_root_y ||
            container.scrollLeft != last_container_x ||
            container.scrollTop != last_container_y) {
          last_changed_frame = frames;
          last_root_x = root.scrollLeft;
          last_root_y = root.scrollTop;
          last_container_x = container.scrollLeft;
          last_container_y = container.scrollTop;
        }
        requestAnimationFrame(tick.bind(this, frames + 1));
      }
    }
    tick(0);
  });
}

function inject_input() {
  return smoothScroll(200, 200, 500, 'up', 4000).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 500, 'up', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 500, 'up', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 100, 'up', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return smoothScroll(200, 200, 100, 'left', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  });
}