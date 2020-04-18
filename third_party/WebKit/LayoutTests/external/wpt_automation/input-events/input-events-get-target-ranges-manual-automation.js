importAutomationScript('/input-events/inputevent_common_input.js');

function inject_input() {
  let isMacOSX = navigator.userAgent.indexOf("Mac OS X") != -1;
  return collapseEndAndKeyDown('#test1_editable', 'Backspace', [isMacOSX ? 'altKey' : 'ctrlKey']).then(() => {
    return collapseEndAndKeyDown('#test2_editable', 'a');
  }).then(() => {
    return selectAndKeyDown('#test2_editable', 'b');
  }).then(() => {
    selectAndExecCommand('#test2_editable', 'bold');
    return focusAndKeyDown('#test3_plain', 'a');
  }).then(() => {
    return keyDown('Backspace');
  });
}
