importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return twoPointerDragInTarget('touch', '#black', 'down').then(function() {
    return touchTapInTarget('#done');
  }).then(function() {
    return twoPointerDragInTarget('touch', '#grey', 'down');
  }).then(function() {
    return touchTapInTarget('#done');
  });
}
