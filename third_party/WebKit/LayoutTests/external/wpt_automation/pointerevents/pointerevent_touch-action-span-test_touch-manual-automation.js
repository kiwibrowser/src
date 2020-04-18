importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
   return touchScrollInTarget('#target0', 'down').then(function() {
    return touchScrollInTarget('#target0', 'right');
  }).then(function() {
    return delayPromise(4*scrollReturnInterval);
  }).then(function() {
    return touchScrollInTarget('#target0 > span', 'down');
  }).then(function() {
    return touchScrollInTarget('#target0 > span', 'right');
  }).then(function() {
    touchTapInTarget('#btnComplete');
  });
}
