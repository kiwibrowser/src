importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseClickInTarget('#target').then(function() {
    return mouseClickInTarget('#done');
  }).then(function() {
    return touchTapInTarget('#target');
  }).then(function() {
    return touchTapInTarget('#done');
  }).then(function() {
    return penClickInTarget('#target')
  }).then(function() {
    return penClickInTarget('#done')
  });
}
