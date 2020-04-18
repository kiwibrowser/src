importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return pointerDragInTarget('mouse', '#btnCapture', 'right');
}

