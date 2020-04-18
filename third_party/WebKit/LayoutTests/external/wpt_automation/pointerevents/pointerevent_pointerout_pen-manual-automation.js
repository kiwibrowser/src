importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return penMoveIntoTarget('#target0').then(function() {
    penMoveToDocument();
  });
}

