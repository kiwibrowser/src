const delta_to_scroll = 100;

function touchScroll(direction, start_x, start_y) {
  if (!window.chrome || !window.chrome.gpuBenchmarking)
    return Promise.reject();

  let delta_x = (direction === "left") ? delta_to_scroll :
                (direction === "right") ? -delta_to_scroll : 0;
  let delta_y = (delta_x !== 0) ? 0 :
                (direction === "up") ? delta_to_scroll :
                (direction === "down") ? -delta_to_scroll : 0;
  if (delta_x === delta_y)
    return Promise.reject("Invalid touch direction.");

  return new Promise((resolve) => {
    chrome.gpuBenchmarking.pointerActionSequence( [
        {source: "touch",
         actions: [
            { name: "pointerDown", x: start_x, y: start_y},
            { name: "pointerMove",
              x: (start_x + delta_x),
              y: (start_y + delta_y)
            },
            { name: "pause", duration: 0.1 },
            { name: "pointerUp" }
        ]}], resolve);
  });
}

window.touch_scroll_api_ready = true;
if (window.resolve_on_touch_scroll_api_ready)
  window.resolve_on_touch_scroll_api_ready();
