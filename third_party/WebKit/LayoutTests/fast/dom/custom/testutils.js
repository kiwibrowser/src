(function () {

function isParentFrame() {
    return window.parent === window;
}

var childDoneMessage = 'PASS child done';

fork = function () {
    if (isParentFrame()) {
        window.addEventListener('message', function (event) {
            debug(event.data);
            if (event.data == childDoneMessage)
                finishJSTest();
        });

        var iframe = document.createElement('iframe');
        iframe.src = window.location;
        document.body.appendChild(iframe);
        iframe = null;
    }

    return isParentFrame();
};

if (!isParentFrame()) {
    var parent = window.parent;
    log = function (msg) {
        parent.postMessage(msg, '*');
    };

    done = function () {
        log(childDoneMessage);
    };

    destroyContext = function () {
        // This function can be called more than once so we need to check whether the iframe exists.
        var frame = parent.document.querySelector('iframe');
        if (frame) {
            frame.remove();
            log('PASS destroyed context');
        }
    };
}

withFrame = function (f) {
    var frame = document.createElement('iframe');
    frame.onload = function () {
        try {
            f(frame);
        } catch (e) {
            testFailed(e);
        }
    };
    frame.src = 'about:blank';
    document.body.appendChild(frame);
};

})();
