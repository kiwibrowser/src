var PerfHarness = (function(undefined) {

  var countHistory = [];
  var totalCount = 0;
  var countIndex = 0;
  var count = 1;
  var velocity = 1;
  var direction = 1;
  var framesToAverage;
  var targetTime;
  var then;
  var callback;
  var canvas;

  var test = function() {
    var now = Date.now() * 0.001;
    var elapsedTime = now - then;
    then = now;
    var desiredDirection = (elapsedTime < targetTime) ? 1 : -1;
    if (direction != desiredDirection) {
      direction = desiredDirection;
      velocity = Math.max(Math.abs(Math.floor(velocity / 4)), 1) * direction;
    }
    velocity *= 2;
    count += velocity;
    count = Math.max(1, count);

    totalCount -= countHistory[countIndex];
    totalCount += count;
    countHistory[countIndex] = count;
    countIndex = (countIndex + 1) % framesToAverage;

    callback(count, Math.floor(totalCount / framesToAverage), elapsedTime);

    window.requestAnimFrame(test, canvas);
  }

  var start = function(_canvas, _callback, opt_framesToAverage, opt_targetFPS) {
    canvas = _canvas;
    callback = _callback;

    framesToAverage = opt_framesToAverage || 60;
    opt_targetFPS = opt_targetFPS || 57;  // we use 57 instead of 60 since timing is bad.

    for (var ii = 0; ii < framesToAverage; ++ii) {
      countHistory.push(0);
    }

    targetTime = 1 / opt_targetFPS;

    then = Date.now() * 0.001;

    test();
  }

  return {
    start: start
  };
}());
