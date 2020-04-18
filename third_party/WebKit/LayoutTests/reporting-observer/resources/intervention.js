function causeIntervention() {
  var target = document.getElementById('target');
  var rect = target.getBoundingClientRect();
  var targetX = rect.left + rect.width / 2;
  var targetY = rect.top + rect.height / 2;

  document.body.addEventListener('touchstart', function(e) {
    e.preventDefault();
  });

  var touches = [new Touch({identifier: 1, clientX: targetX, clientY: targetY, target: target})];
  var touchEventInit = {
    cancelable: false,
    touches: touches,
    targetTouches: touches,
    changedTouches: touches,
    view: window
  };
  var event = new TouchEvent('touchstart', touchEventInit);

  var deadline = performance.now() + 100;
  while (performance.now() < deadline) {};

  document.body.dispatchEvent(event);
}
