<!DOCTYPE html>
<meta name="viewport" content="width=device-width, user-scalable=no">
<link rel="stylesheet" type="text/css" href="resources/tablet.css">
<link rel="stylesheet" type="text/css" href="resources/transition.css">
<script src="resources/perf_test_helper.js"></script>

<container id="container"></container>

<script>
var N = PerfTestHelper.getN(1000);
var duration = 1000;
var style = null;
var keyframe = 1;
var keyframeValues = [0, 1]

for (var i = 0; i < N; i++) {
  container.appendChild(document.createElement('target'));
}

function startAllTransitions() {
  keyframe ^= 1;
  if (style) {
    style.remove();
  }
  style = document.createElement('style');
  style.textContent = 'target { opacity: ' + keyframeValues[keyframe] + '; }';
  container.appendChild(style);
}

requestAnimationFrame(startAllTransitions);
setInterval(startAllTransitions, duration);

PerfTestHelper.signalReady();
</script>
