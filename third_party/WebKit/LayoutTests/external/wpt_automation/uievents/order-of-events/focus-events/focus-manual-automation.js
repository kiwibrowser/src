importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseClickInTarget('#a').then(function() {
    return mouseClickInTarget('#b');
  }).then(function() {
    return mouseClickInTarget('#done');
  });
}