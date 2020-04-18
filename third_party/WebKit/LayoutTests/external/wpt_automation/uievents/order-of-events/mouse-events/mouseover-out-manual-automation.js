importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseMoveIntoTarget('#outer').then(function() {
    return mouseMoveIntoTarget('#inner');
  }).then(function() {
    return mouseMoveIntoTarget('#released');
  }).then(function() {
    return mouseMoveIntoTarget('#inner');
  }).then(function() {
    return mouseMoveIntoTarget('#outer');
  }).then(function() {
    return mouseMoveToDocument();
  });
}