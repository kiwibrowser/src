importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseClickInTarget('#target0').then(function() {
    return keyboardScroll('down');
  }).then(function() {
    return keyboardScroll('right');
  });
}

