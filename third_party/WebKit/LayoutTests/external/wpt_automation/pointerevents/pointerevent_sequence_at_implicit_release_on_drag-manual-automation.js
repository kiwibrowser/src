importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return pointerDragInTarget('touch', '#target', 'right').then(function() {
    return touchTapInTarget('#done');
  });
}
