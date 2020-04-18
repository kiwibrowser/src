<html><head>
<title>Ping</title>
<script>

var testCalled = false;

function test() {
    if (!testCalled) {
        if (window.testRunner) {
            testRunner.dumpAsText();
            testRunner.overridePreference("WebKitHyperlinkAuditingEnabled", 1);
            testRunner.waitUntilDone();
            testRunner.dumpPingLoaderCallbacks();
            testRunner.overridePreference("WebKitAllowRunningInsecureContent", false);
            testRunner.setAllowRunningOfInsecureContent(false);
        }
        testCalled = true;
        return;
    }

    if (window.eventSender) {
        var a = document.getElementById("a");
        eventSender.mouseMoveTo(a.offsetLeft + 2, a.offsetTop + 2);
        eventSender.mouseDown();
        eventSender.mouseUp();
    }
}

</script>
</head>
<body onload="test();">
<img src="resources/delete-ping.php?test=cross-origin-from-https" onload="test();" onerror="test();"></img>
<a id="a" href="/navigation/resources/notify-done.html" ping="http://example.test:8000/navigation/resources/save-Ping.php?test=cross-origin-from-https">Navigate and send ping</a>
</body></html>
