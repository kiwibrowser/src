importAutomationScript('/input-events/inputevent_common_input.js');

function inject_input() {
  return focusAndKeyDown('#plain', 'a').then(() => {
    return keyDown('B', ['shiftKey']);
  }).then(() => {
    return focusAndKeyDown('#rich', 'c');
  }).then(() => {
    return keyDown('D', ['shiftKey']);
  });
}
