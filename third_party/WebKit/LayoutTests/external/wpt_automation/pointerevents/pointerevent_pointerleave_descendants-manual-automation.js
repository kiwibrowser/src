importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseMoveIntoTarget('#target0').then(function() {
    mouseMoveIntoTarget('#target0 > div')
  }).then(function() {
    return mouseMoveToDocument();
  });
}

