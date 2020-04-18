var global = window;

function rejectIfThrown(func, reject) {
    return function() {
        try {
            func();
        } catch (e) {
            reject(e);
        }
    };
}

new Promise(function(resolve, reject) {
    global.req = new XMLHttpRequest;
    req.responseType = 'text';
    req.open('GET', 'resources/get.txt', true);
    req.onreadystatechange = rejectIfThrown(function() {
        if (req.readyState != req.DONE) {
            return;
        }

        shouldBe('req.status', '200');
        shouldBeEqualToString('req.response', 'PASS');
        resolve();
    }, reject);
    req.send(null);
}).catch(function(r) {
    testFailed('Set responseType before open(): Failed: ' + r);
}).then(function() {
    return new Promise(function(resolve, reject)
    {
        global.req = new XMLHttpRequest;
        req.responseType = 'blob';
        req.open('GET', 'resources/get.txt', true);
        req.onreadystatechange = rejectIfThrown(function() {
            if (req.readyState != req.DONE) {
                return;
            }

            shouldBe('req.status', '200');
            shouldBeEqualToString('req.response', 'PASS');
            resolve();
        }, reject);
        req.responseType = 'text';
        req.send(null);
    });
}).catch(function(r) {
    testFailed('Change responseType from blob to text between open() and send(): Failed: ' + r);
}).then(function() {
    return new Promise(function(resolve, reject)
    {
        global.req = new XMLHttpRequest;
        req.responseType = 'blob';
        req.open('GET', 'resources/get.txt', true);
        req.onreadystatechange = rejectIfThrown(function() {
            if (req.readyState == req.HEADERS_RECEIVED) {
                req.responseType = 'text';
                return;
            } else if (req.readyState != req.DONE) {
                return;
            }

            shouldBe('req.status', '200');
            shouldBeEqualToString('req.response', 'PASS');
            resolve();
        }, reject);
        req.send(null);
    });
}).catch(function(r) {
    testFailed('Change responseType from blob to text in HEADERS_RECEIVED: Failed: ' + r);
}).then(finishJSTest, finishJSTest);
