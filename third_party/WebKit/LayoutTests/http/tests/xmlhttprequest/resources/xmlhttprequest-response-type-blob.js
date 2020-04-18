var global = window;

function readBlobAsArrayBuffer(blob) {
    return new Promise(function (resolve, reject) {
        var reader = new FileReader();
        reader.onload = function (evt) {
            resolve(evt.target.result);
        };
        reader.onabort = function (evt) {
            reject(evt);
        };
        reader.onerror = function (evt) {
            reject(evt);
        };
        reader.readAsText(blob, "utf-8");
    });
}

new Promise(function (resolve, reject) {
    global.req = new XMLHttpRequest;
    req.responseType = "blob";
    req.open("GET", "resources/get.txt", true);
    req.onreadystatechange = function () {
        if (req.readyState != req.DONE) {
            return;
        }

        shouldBe("req.status", "200");
        shouldBe("req.response.size", "4");
        readBlobAsArrayBuffer(req.response).then(function (value) {
            global.buffer = value;
            shouldBeEqualToString("buffer", "PASS");
            testPassed("Set responseType before open(): Successful");
        }).then(resolve, reject);
    };
    req.send(null);
}).catch(function (r) {
    testFailed("Set responseType before open: Failed: " + r);
}).then(function () {
    return new Promise(function (resolve, reject)
    {
        global.req = new XMLHttpRequest;
        req.open('GET', 'resources/get.txt', true);
        req.onreadystatechange = function () {
            if (req.readyState != req.DONE) {
                return;
            }

            shouldBe("req.status", "200");
            shouldBe("req.response.size", "4");
            readBlobAsArrayBuffer(req.response).then(function (value) {
                global.buffer = value;
                shouldBeEqualToString("buffer", "PASS");
                testPassed("Set responseType before send(): Successful");
            }).then(resolve, reject);
        };
        req.responseType = 'blob';
        req.send(null);
    });
}).catch(function (r) {
    testFailed("Set responseType before send(): Failed: " + r);
}).then(function () {
    return new Promise(function (resolve, reject)
    {
        global.req = new XMLHttpRequest;
        req.open('GET', 'resources/get.txt', true);
        req.onreadystatechange = function () {
            if (req.readyState == req.HEADERS_RECEIVED) {
                req.responseType = 'blob';
                return;
            } else if (req.readyState != req.DONE) {
                return;
            }

            shouldBe("req.status", "200");
            shouldBe("req.response.size", "4");
            readBlobAsArrayBuffer(req.response).then(function (value) {
                global.buffer = value;
                shouldBeEqualToString("buffer", "PASS");
                testPassed("Set responseType in HEADERS_RECEIVED: Successful");
            }).then(finishJSTest, reject);
        };
        req.send(null);
    });
}).catch(function (r) {
    testFailed("Set responseType in HEADERS_RECEIVED: Failed: " + r);
    finishJSTest();
});
