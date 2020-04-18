importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseClickInTarget('#square1').then(function() {
    return mouseClickInTarget('#square2', document.querySelector('#innerFrame'));
  }).then(function() {
    return mouseMoveToDocument();
  }).then(function() {
    return penClickInTarget('#square1');
  }).then(function() {
    return penClickInTarget('#square2', document.querySelector('#innerFrame'));
  }).then(function() {
    return penMoveToDocument();
  });
}
