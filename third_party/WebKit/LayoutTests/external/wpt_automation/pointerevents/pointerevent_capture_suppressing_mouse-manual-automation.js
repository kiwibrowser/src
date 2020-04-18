importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseMoveIntoTarget('#target0').then(function() {
    return mouseMoveIntoTarget('#target1');
  }).then(function() {
    return mouseDragInTargets(['#btnCapture', '#btnCapture', '#target1', '#target0', '#target1']);
  }).then(function() {
    return mouseMoveIntoTarget('#target1');
  });
}

