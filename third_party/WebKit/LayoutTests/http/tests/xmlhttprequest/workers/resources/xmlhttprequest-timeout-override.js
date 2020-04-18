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
var worker = createWorker('resources/xmlhttprequest-timeout-override-worker.js');
var messages = [];
worker.onmessage = function(evt)
{
    if (/log .+/.test(evt.data)) {
        var msg = evt.data.substr(4);
        messages.push(msg);
        log(msg);
    } else if (/DONE/.test(evt.data)) {
        log(messages.length === 1 && messages[0] === "timeout" ? "PASS" : "FAIL");
        if (window.testRunner)
            testRunner.notifyDone();
    }
}
