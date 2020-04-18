importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return smoothDrag('#box1', '#box2', 'mouse').then(function() {
    return smoothDrag('#box1', '#box2', 'touch');
  });
}
