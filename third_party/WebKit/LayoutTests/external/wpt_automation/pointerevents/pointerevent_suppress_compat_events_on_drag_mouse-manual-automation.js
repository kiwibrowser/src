importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseDragInTarget('#target0').then(function() {
    return mouseDragInTarget('#target1');
  }).then(function() {
    return mouseClickInTarget('#done');
  });
}
