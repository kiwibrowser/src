description("This tests if mouse events are dispatched on an element obscured by a pseudo-element.");

var div = document.createElement("div");
div.id = "square";

var eventLog = "";

function appendEventLog() {
    if (window.eventSender)
        eventLog += event.type + " ";
    else
        debug(event.type);
}

function clearEventLog() {
    eventLog = "";
}

div.addEventListener("click", appendEventLog, false);
div.addEventListener("dblclick", appendEventLog, false);
div.addEventListener("mousedown", appendEventLog, false);
div.addEventListener("mouseup", appendEventLog, false);

document.body.insertBefore(div, document.body.firstChild);

function sendEvents(button) {
}

function testEvents(description, button, expectedString) {
    debug(description);
    sendEvents(button);
}

if (!window.eventSender) {
    debug("This test requires DumpRenderTree.  Click on the gray rect with left mouse button to log.")
} else {
    var button = 0;
    eventSender.mouseMoveTo(10, 10);
    eventSender.mouseDown(button);
    eventSender.mouseUp(button);
    eventSender.mouseDown(button);
    eventSender.mouseUp(button);
    shouldBeEqualToString("eventLog", "mousedown mouseup click mousedown mouseup click dblclick ");
    clearEventLog();
}