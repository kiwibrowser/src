description("Tests basic use of GestureFlingStart");

var actualWheelEventsOccurred = 0;
var cumulativeScrollX = 0;
var cumulativeScrollY = 0;

var minimumWheelEventsExpected = "2";
var minimumScrollXExpected = 300;
var minimumScrollYExpected = 300;

var positionX = 10;
var positionY = 11;
var velocityX = 10000;
var velocityY = 10000;

function recordWheelEvent(event)
{
    if (event.clientX != 10)
      debug('FAIL: clientX != 10');

    if (event.clientY != 11)
      debug('FAIL: event.clientY != 11');

    actualWheelEventsOccurred++;
    cumulativeScrollX += event.wheelDeltaX;
    cumulativeScrollY += event.wheelDeltaY;

    if (cumulativeScrollX >= minimumScrollXExpected
            && cumulativeScrollY >= minimumScrollYExpected) {
      shouldBeGreaterThanOrEqual('actualWheelEventsOccurred', minimumWheelEventsExpected);
      shouldBeGreaterThanOrEqual('cumulativeScrollX', minimumScrollXExpected.toString());
      shouldBeGreaterThanOrEqual('cumulativeScrollY', minimumScrollYExpected.toString());

      isSuccessfullyParsed();
      if (window.testRunner)
          testRunner.notifyDone();
    }
    event.preventDefault();
}

document.addEventListener("mousewheel", recordWheelEvent);

if (window.testRunner && window.eventSender && eventSender.gestureFlingStart) {
    // At least one wheel event must happen before touchpad fling start.
    eventSender.mouseMoveTo(10, 11);
    eventSender.mouseScrollBy(1, 1, false, true, 0, true, "phaseBegan");
    eventSender.gestureFlingStart(positionX, positionY, velocityX, velocityY, "touchpad");
}

if (window.testRunner)
    testRunner.waitUntilDone();
