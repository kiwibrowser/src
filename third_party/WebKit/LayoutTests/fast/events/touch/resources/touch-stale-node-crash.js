document.ontouchstart = touchStartHandler;

function touchStartHandler(e)
{
    var target = e.touches[0].target;
    document.body.removeChild(target);
}

description("If this test does not crash then you pass!");

if (window.testRunner)
    testRunner.waitUntilDone();

if (window.eventSender) {
    eventSender.clearTouchPoints();
    eventSender.addTouchPoint(50, 150);
    eventSender.touchStart();
    window.location = 'resources/send-touch-up.html';
} else
    debug('This test requires DRT.');
