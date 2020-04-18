// NOTE: You must include resources/js-test.js before this file in a test case since
// this file makes use of functions in js-test.js.

var replayEventQueue = []; // Global queue of recorded events.
var registeredElementsAndEventsStruct; // Global structure of registered elements and events.

function registerElementsAndEventsToRecord(elementsToRecord, eventsToRecord)
{
    registeredElementsAndEventsStruct = {"elementsToRecord": elementsToRecord, "eventsToRecord": eventsToRecord};
}

function beginRecordingEvents()
{
    function callback(element, eventName)
    {
        element.addEventListener(eventName, _recordEvent, false);
    }
    _processEachRegisteredElement(callback);
}

function endRecordingEvents()
{
    function callback(element, eventName)
    {
        element.removeEventListener(eventName, _recordEvent, false);
    }
    _processEachRegisteredElement(callback);
}

function _processEachRegisteredElement(callback)
{
    if (!registeredElementsAndEventsStruct)
        return;
    var elements = registeredElementsAndEventsStruct.elementsToRecord;
    var events = registeredElementsAndEventsStruct.eventsToRecord;
    for (var i = 0; i < elements.length; ++i) {
        for (var j = 0; j < events.length; ++j)
            callback(elements[i], events[j])
    }
}

function _recordEvent(event)
{
    replayEventQueue.push([event, event.currentTarget.id]);
}

function checkThatEventsFiredInOrder(expectedOrderQueue)
{
    while (replayEventQueue.length && expectedOrderQueue.length) {
        var replayedEvent = replayEventQueue.shift();
        var expectedEvent = expectedOrderQueue.shift();
        var replayedEventTargetName = replayedEvent[1];
        if (replayedEventTargetName === expectedEvent[0] && replayedEvent[0].type === expectedEvent[1])
            testPassed('fired event is (' + replayedEventTargetName + ', ' + replayedEvent[0].type + ').');
        else {
            testFailed('fired event is (' + replayedEventTargetName + ', ' + replayedEvent[0].type + '). ' +
                       'Should be (' + expectedEvent[0] + ', ' + expectedEvent[1] + ').');
        }
    }
    while (replayEventQueue.length) {
        var replayedEvent = replayEventQueue.shift();
        testFailed('should not have fired event (' + replayedEvent[1] + ', ' + replayedEvent[0].type + '). But did.');
    }
    while (expectedOrderQueue.length) {
        var expectedEvent = expectedOrderQueue.shift();
        testFailed('should have fired event (' + expectedEvent[0] + ', ' + expectedEvent[1] + '). But did not.');
    }
}
