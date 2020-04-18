function onmessage(evt)
{
    var target = self;
    target.addEventListener('custom-event', function(e) {
        postMessage("SUCCESS");
    }, true);

    var event = new Event('custom-event');
    target.dispatchEvent(event);
}

addEventListener("message", onmessage, true);
