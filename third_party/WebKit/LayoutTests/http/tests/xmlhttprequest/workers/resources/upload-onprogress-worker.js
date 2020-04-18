importScripts("worker-pre.js");

function log(message)
{
    postMessage("log " + message);
}

function onLoad()
{
    postMessage("DONE");
}

function onProgress(e)
{
    postMessage("tick " + e.loaded);
}

function init()
{
    try {
        var xhr = new XMLHttpRequest();
        xhr.upload.onprogress = onProgress;
        xhr.onload = onLoad;
        xhr.open("POST", "../../resources/post-echo.cgi");
        xhr.send((new Array(100000)).join("aa"));
    } catch (e) {
        log("Exception received.");
    }
}
