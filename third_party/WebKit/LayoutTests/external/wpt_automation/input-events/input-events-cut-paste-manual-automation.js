importAutomationScript('/input-events/inputevent_common_input.js');

function inject_input() {
  return selectAndKeyDown('#test1_plain', 'Cut').then(() => {
    return keyDown('Paste');
  }).then(() => {
    return selectAndKeyDown('#test2_editable', 'Cut');
  }).then(() => {
    return keyDown('Paste');
  }).then(() => {
    return selectAndKeyDown('#test3_editable_prevent', 'Paste');
  }).then(() => {
    return selectAndKeyDown('#test3_editable_prevent', 'Cut');
  }).then(() => {
    return selectAndKeyDown('#test3_editable_normal', 'Paste');
  });
}
