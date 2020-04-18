importScripts("worker-pre.js");

function log(message) {
    postMessage("log " + message);
}

function done() {
    postMessage("DONE");
}

function eventHandler(e) {
    log(e.type);
    done();
}

function init() {
    try {
        var xhr = new XMLHttpRequest();
        xhr.ontimeout = eventHandler;
        xhr.onabort = eventHandler;
        xhr.onerror = eventHandler;
        xhr.onload = eventHandler;

        xhr.timeout = 100000;
        xhr.open("GET", "../../../resources/load-and-stall.php?name=../resources/test.mp4&stallAt=0&stallFor=1000&mimeType=video/mp4", true);

        // Defer overriding timeout
        setTimeout(function() {
            xhr.timeout = 400;
        }, 200);

        setTimeout(function() {
            xhr.abort();
        }, 1000);

        xhr.send();
    } catch (e) {
        log(e);
        done();
    }
};
