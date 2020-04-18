importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseClickInTarget('#target', undefined, 'right').then(function() {
    return mouseClickInTarget('#done');
  });
}
