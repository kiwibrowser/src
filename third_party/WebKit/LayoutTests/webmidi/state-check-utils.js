function checkStateTransition(options) {
    debug("Check state transition for " + options.method + " on " +
          options.initialconnection + " state.");
    debug("- check initial state.");
    window.port = options.port;
    shouldBeEqualToString("port.connection", options.initialconnection);
    var checkHandler = function(e) {
        window.eventport = e.port;
        testPassed("handler is called with port " + eventport + ".");
        if (options.initialconnection == options.finalconnection) {
            testFailed("onstatechange handler should not be called here.");
        }
        shouldBeEqualToString("eventport.id", options.port.id);
        shouldBeEqualToString("eventport.connection", options.finalconnection);
    };
    const portPromise = new Promise(resolve => {
        port.onstatechange = e => {
            debug("- check port handler.");
            checkHandler(e);
            resolve();
        };
    });
    const accessPromise = new Promise(resolve => {
        access.onstatechange = e => {
            debug("- check access handler.");
            checkHandler(e);
            resolve();
        };
    });
    if (options.method == "setonmidimessage") {
        port.onmidimessage = function() {};
        return Promise.all([portPromise, accessPromise]);
    }
    if (options.method == "addeventlistener") {
        port.addEventListener("midimessage", function() {});
        return Promise.all([portPromise, accessPromise]);
    }
    if (options.method == "send") {
        port.send([]);
        return Promise.all([portPromise, accessPromise]);
    }
    // |method| is expected to be "open" or "close".
    return port[options.method]().then(function(p) {
        window.callbackport = p;
        debug("- check callback arguments.");
        testPassed("callback is called with port " + callbackport + ".");
        shouldBeEqualToString("callbackport.id", options.port.id);
        shouldBeEqualToString("callbackport.connection", options.finalconnection);
        debug("- check final state.");
        shouldBeEqualToString("port.connection", options.finalconnection);
    }, function(e) {
        testFailed("error callback should not be called here.");
        throw e;
    });
}


