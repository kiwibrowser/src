function stripURL(url) {
    return url ? url.match( /[^\/]+\/?$/ )[0] : url;
}

function throwException(message) {
    throw new Error(message ? message : "An exception");
}

var errorsSeen = 0;
function dumpOnErrorArgumentValuesAndReturn(returnValue, callback) {
    window.onerror = function (message, url, line, column, error) {
        debug("window.onerror: \"" + message + "\" at " + stripURL(url) + " (Line: " + line + ", Column: " + column + ")");
        if (error)
            debug(stripStackURLs(error.stack));
        else
            debug("No stack trace.");

        if (callback)
            callback(++errorsSeen);
        if (returnValue)
            debug("Returning 'true': the error should not be reported in the console as an unhandled exception.\n\n\n");
        else
            debug("Returning 'false': the error should be reported in the console as an unhandled exception.\n\n\n");
        return returnValue;
    };
}

function dumpErrorEventAndAllowDefault(callback) {
    window.addEventListener('error', function (e) {
        dumpErrorEvent(e)
        debug("Not calling e.preventDefault(): the error should be reported in the console as an unhandled exception.\n\n\n");
        if (callback)
            callback(++errorsSeen);
    });
}

function dumpErrorEventAndPreventDefault(callback) {
    window.addEventListener('error', function (e) {
        dumpErrorEvent(e);
        debug("Calling e.preventDefault(): the error should not be reported in the console as an unhandled exception.\n\n\n");
        e.preventDefault();
        if (callback)
            callback(++errorsSeen);
    });
}

var eventPassedToTheErrorListener = null;
var eventCurrentTarget = null;
function dumpErrorEvent(e) {
    debug("Handling '" + e.type + "' event (phase " + e.eventPhase + "): \"" + e.message + "\" at " + stripURL(e.filename) + ":" + e.lineno);
    if (e.error)
        debug(stripStackURLs(e.error.stack));
    else
        debug("No stack trace.");

    eventPassedToTheErrorListener = e;
    eventCurrentTarget = e.currentTarget;
    shouldBe('eventPassedToTheErrorListener', 'window.event');
    shouldBe('eventCurrentTarget', 'window');
    eventPassedToTheErrorListener = null;
    eventCurrentTarget = null;
}

function stripStackURLs(stackTrace) {
    stackTrace = stackTrace.split("\n");
    var length = Math.min(stackTrace.length, 100);
    var text = "Stack Trace:\n";
    for (var i = 0; i < length; i++) {
        text += stackTrace[i].replace(/at ((?:eval at \()?[a-zA-Z\.]+ )?\(?.+\/([^\/]+):(\d+):(\d+)\)?/, "at $1$2:$3:$4") + "\n";
    }
    return text;
}
