function repaintTest() {
    if (!window.eventSender)
        return;

    for (i = 0; i < zoomCount; ++i) {
        if (window.shouldZoomOut)
            eventSender.zoomPageOut();
        else
            eventSender.zoomPageIn();
    }

    if (!window.postZoomCallback)
        return;

    window.jsTestIsAsync = true;
    if (window.testRunner)
        testRunner.waitUntilDone();

    window.postZoomCallback();
    finishJSTest();
}
