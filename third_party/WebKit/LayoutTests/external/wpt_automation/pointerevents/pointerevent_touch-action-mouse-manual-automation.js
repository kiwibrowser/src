importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseWheelScroll('#target0', 'down').then(function() {
    return mouseWheelScroll('#target0', 'right');
  });
}

