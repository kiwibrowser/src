function onmessage(evt)
{
    postMessage("SUCCESS");
}

addEventListener("message", onmessage, true);
gc();
