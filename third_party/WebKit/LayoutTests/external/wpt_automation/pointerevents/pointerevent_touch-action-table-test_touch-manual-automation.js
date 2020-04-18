importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
   return touchScrollInTarget('#row1', 'down').then(function() {
    return touchScrollInTarget('#row1', 'right');
  }).then(function() {
    return delayPromise(4*scrollReturnInterval);
  }).then(function() {
    return touchScrollInTarget('#cell3', 'down');
  }).then(function() {
    return touchScrollInTarget('#cell3', 'right');
  }).then(function() {
    touchTapInTarget('#btnComplete');
  });
}
