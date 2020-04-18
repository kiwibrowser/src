importScripts("worker-pre.js");

onmessage = function(event)
{
   if (event.data == "START")
       start();
}

function log(message)
{
    postMessage("log " + message);
}

function done()
{
    postMessage("DONE");
}

function start()
{
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "http://localhost:8000/xmlhttprequest/workers/resources/access-control-preflight-request-headers-referer.php");
    // Add a non-simple header to make CORS preflight happen.
    xhr.setRequestHeader("X-Custom-Header", "PASS");
    xhr.onerror = function () {
        log("FAIL");
        done();
    };
    xhr.onload = function () {
        log(xhr.responseText);
        done();
    };
    xhr.send();
}
