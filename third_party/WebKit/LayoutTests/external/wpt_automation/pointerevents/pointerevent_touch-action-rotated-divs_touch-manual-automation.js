importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return touchScrollInTarget('#target', 'down').then(function() {
    return touchScrollInTarget('#target', 'up');
  }).then(function() {
    return touchScrollInTarget('#target', 'right');
  }).then(function() {
    return touchScrollInTarget('#target', 'left');
  }).then(function() {
    return touchTapInTarget('#btnDone');
  }).then(function() {
    return touchScrollInTarget('#target', 'down');
  }).then(function() {
    return touchScrollInTarget('#target', 'up');
  }).then(function() {
    return touchScrollInTarget('#target', 'right');
  }).then(function() {
    return touchScrollInTarget('#target', 'left');
  }).then(function() {
    return touchTapInTarget('#btnDone');
  });
}

