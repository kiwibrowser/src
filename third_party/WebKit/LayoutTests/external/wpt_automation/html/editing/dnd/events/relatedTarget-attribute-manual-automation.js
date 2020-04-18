importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
    return mouseDragAndDropInTargets(['#draggable', '#outerdiv', '#innerdiv', '#outerdiv']);
}

