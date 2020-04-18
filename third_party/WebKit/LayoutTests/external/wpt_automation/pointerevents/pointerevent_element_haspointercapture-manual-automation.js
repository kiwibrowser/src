importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseDragInTargets(['#target0', '#target1']).then(function() {
    return mouseClickInTarget('#target1');
  });
}
