importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return touchTapInTarget('#square1').then(function() {
    return touchTapInTarget('#square2', document.querySelector('#innerFrame'));
  });
}
