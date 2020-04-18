function log(message)
{
    if (self.importScripts) {
        postMessage(message);
    } else {
        document.getElementById('console').appendChild(document.createTextNode(message + "\n"));
    }
}

var uuidRegex = new RegExp('[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}');

function replaceUUID(s)
{
    return s.replace(uuidRegex, 'UUID');
}

function sendXMLHttpRequestSync(method, url)
{
    var xhr = new XMLHttpRequest();
    xhr.open(method, url, false);
    try {
        xhr.send();
        log("Status: " + xhr.status);
        log("Response: " + xhr.responseText);
    } catch (error) {
        log("Received exception, code: " + error.code + ", name: " + error.name + ", message: " + replaceUUID(error.message));
    }
}

function sendXMLHttpRequestAsync(method, url)
{
    return new Promise(function (resolve) {
        var xhr = new XMLHttpRequest();

        xhr.onload = function()
        {
            log("Status: " + xhr.status);
            log("Response: " + xhr.responseText);
        };
        xhr.onerror = function()
        {
            log("Error event is dispatched");
        };
        xhr.onloadend = function()
        {
            resolve();
        };

        xhr.open(method, url, true);
        try {
            xhr.send();
        } catch (error) {
            log("Received exception, code: " + error.code + ", name: " + error.name + ", message: " + replaceUUID(error.message));
        }
    });
}

function sendXMLHttpRequestAsyncWithBlobSlice(method, url)
{
    return new Promise(function (resolve) {
        var xhr = new XMLHttpRequest();
        xhr.responseType = 'blob';

        var blob_read = false;
        var onloadend_called = false;
        xhr.onload = function()
        {
            log("Status: " + xhr.status);
            var blob = xhr.response.slice(0, 1);
            var reader = new FileReader();
            reader.addEventListener("loadend", function()
            {
                log("First byte of response: " + reader.result);
                blob_read = true;
                if (onloadend_called)
                    resolve();
            });
            reader.readAsText(blob);
        };
        xhr.onerror = function()
        {
            log("Error event is dispatched");
        };
        xhr.onloadend = function()
        {
            onloadend_called = true;
            if (blob_read)
                resolve();
        };

        xhr.open(method, url, true);
        try {
            xhr.send();
        } catch (error) {
            log("Received exception, code: " + error.code + ", name: " + error.name + ", message: " + replaceUUID(error.message));
        }
    });
}

function runXHRs(file)
{
    var fileURL = URL.createObjectURL(file);

    log("Test that sync XMLHttpRequest GET succeeds.");
    sendXMLHttpRequestSync("GET", fileURL);

    log("Test that sync XMLHttpRequest POST fails.");
    sendXMLHttpRequestSync("POST", fileURL);

    log("Test that sync XMLHttpRequest GET fails after the blob URL is revoked.");
    URL.revokeObjectURL(fileURL);
    sendXMLHttpRequestSync("GET", fileURL);

    fileURL = URL.createObjectURL(file);

    log("Test that async XMLHttpRequest GET succeeds.");
    sendXMLHttpRequestAsync("GET", fileURL).then(function()
    {
        log("Test that async XMLHttpRequest POST fails.");
        return sendXMLHttpRequestAsync("POST", fileURL);
    }).then(function()
    {
        log("Test the slicing the blob response doesn't crash the browser.")
        return sendXMLHttpRequestAsyncWithBlobSlice("GET", fileURL);
    }).then(function()
    {
        log("Test that async XMLHttpRequest GET fails after the blob URL is revoked.");
        URL.revokeObjectURL(fileURL);
        return sendXMLHttpRequestAsync("GET", fileURL);
    }).then(function()
    {
        log("DONE");
        if (!self.importScripts && testRunner.notifyDone)
            testRunner.notifyDone();
    });
}

if (self.importScripts) {
    onmessage = function(event)
    {
        runXHRs(event.data);
    };
}
