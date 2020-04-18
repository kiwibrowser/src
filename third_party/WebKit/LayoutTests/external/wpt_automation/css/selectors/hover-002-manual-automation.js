importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return mouseMoveIntoTarget("#hovered").then(() => {
    return mouseMoveIntoTarget("#hovered2");
  });
}
