importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseMoveIntoTarget('#target0').then(function() {
    return mouseClickInTarget('#target1');
  });
}

