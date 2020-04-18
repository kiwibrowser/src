importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseDragInTargets(['#lightgreen', '#lightyellow', '#lightblue'], 'middle');
}
