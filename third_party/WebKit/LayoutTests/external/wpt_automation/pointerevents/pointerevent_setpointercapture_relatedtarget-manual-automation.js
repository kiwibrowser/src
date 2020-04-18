importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseMoveIntoTarget('#target1').then(function() {
    return mouseDragInTargets(['#btnCapture', '#target0']);
  });
}

