importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return pointerDragInTarget('touch', '#target0', 'right').then(function() {
    return pointerDragInTarget('touch', '#target1', 'right');
  });
}
