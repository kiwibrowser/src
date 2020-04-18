importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return pointerDragInTarget('mouse', '#target0', 'right').then(function() {
    return pointerDragInTarget('touch', '#target0', 'right');
  }).then(function() {
    return pointerDragInTarget('pen', '#target0', 'right');
  });
}
