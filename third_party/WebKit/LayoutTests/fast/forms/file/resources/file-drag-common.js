function moveMouseToCenterOfElement(element) {
    var centerX = element.offsetLeft + element.offsetWidth / 2;
    var centerY = element.offsetTop + element.offsetHeight / 2;
    eventSender.mouseMoveTo(centerX * devicePixelRatio, centerY * devicePixelRatio);
}

function dragFilesOntoInput(input, files) {
    eventSender.beginDragWithFiles(files);
    moveMouseToCenterOfElement(input);
    eventSender.mouseUp();
}

function dragFilesOntoElement(element, files) {
    eventSender.beginDragWithFiles(files);
    var centerX = element.offsetLeft + element.offsetWidth / 2;
    var centerY = element.offsetTop + element.offsetHeight / 2;
    eventSender.mouseMoveTo(centerX, centerY);
    eventSender.mouseUp();
}
