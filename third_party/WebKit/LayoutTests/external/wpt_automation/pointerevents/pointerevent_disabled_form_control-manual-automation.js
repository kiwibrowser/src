importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseClickInTarget('#target0').then(function() {
    return mouseMoveToDocument();
  }).then(function() {
    return pointerDragInTarget('touch', '#target0', 'right');
  }).then(function() {
    return penClickInTarget('#target0');
  }).then(function() {
    return penMoveToDocument();
  });
}
