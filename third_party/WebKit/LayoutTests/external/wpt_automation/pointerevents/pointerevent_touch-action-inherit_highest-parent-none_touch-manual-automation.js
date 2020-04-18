importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return touchScrollInTarget('#scrollTarget', 'down').then(function() {
    return touchScrollInTarget('#scrollTarget', 'right');
  });
}
