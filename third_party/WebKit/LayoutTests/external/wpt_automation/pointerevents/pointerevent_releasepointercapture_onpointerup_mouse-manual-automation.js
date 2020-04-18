importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseClickInTarget('#btnCapture').then(function() {
    // To Handle delayed capturing.
    return mouseMoveIntoTarget('#btnCapture');
  });
}

