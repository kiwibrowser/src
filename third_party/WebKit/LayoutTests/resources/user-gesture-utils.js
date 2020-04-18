/**
 * Simulates a user click on coordinates [x], [y].
 * For example, for vibrate to be allowed:
 * https://www.chromestatus.com/feature/5644273861001216.
 */
function simulateUserClick(x, y) {
  if (window.eventSender) {
    eventSender.mouseMoveTo(x, y);
    eventSender.mouseDown();
    eventSender.mouseUp();
  }
}

