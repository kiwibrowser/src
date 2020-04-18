importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return touchScrollInTarget('#target0 > div > div', 'down').then(function() {
    return touchScrollInTarget('#target0 > div > div', 'right');
  }).then(function() {
    return touchTapInTarget('#btnComplete');
  });
}
