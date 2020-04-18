if (window.testRunner) {
    testRunner.dumpAsText();
    testRunner.waitUntilDone();
 }

var console_messages = document.createElement("ul");
document.body.appendChild(console_messages);

function log(message)
{
    var item = document.createElement("li");
    item.appendChild(document.createTextNode(message));
    console_messages.appendChild(item);
}

var progress_ticks = 0;
var worker = createWorker('resources/upload-onprogress-worker.js');
worker.onmessage = function(evt)
{
    if (/log .+/.test(evt.data)) {
        log(evt.data.substr(4));
    } else if (/tick .+/.test(evt.data)) {
        progress_ticks++;
    } else if (/DONE/.test(evt.data)) {
        log(progress_ticks >= 1 ? "PASS" : "FAIL");
        if (window.testRunner)
            testRunner.notifyDone();
    }
}
