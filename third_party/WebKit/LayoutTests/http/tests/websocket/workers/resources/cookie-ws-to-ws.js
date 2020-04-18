importScripts('../../resources/get-request-header.js');

if (self.postMessage)
    runTests();
else
    onconnect = handleConnect;

function handleConnect(event)
{
    self.postMessage = function (message) {
        event.ports[0].postMessage(message);
    };
    runTests();
}

function setCookie() {
    return new Promise(function (resolve, reject)
    {
        var opened = false;

        var ws = new WebSocket('ws://127.0.0.1:8880/set-cookie');
        ws.onopen = function () {
            opened = true;
            ws.close();
        };
        ws.onmessage = function (evt) {
            reject('Unexpected message event on set-cookie socket');
        };
        ws.onerror = function () {
            reject('Error on set-cookie socket');
        };
        ws.onclose = function (evt) {
            if (opened)
                resolve();
            else
                reject('Close event handler is called on set-cookie socket before receiving open event');
        };
    });
}

function echoCookie() {
    return connectAndGetRequestHeader('cookie');
}

function runTests()
{
    setCookie()
        .then(echoCookie)
        .then(
        function (cookie) {
            if (cookie != 'ws-domain-local-ip=1; ws-path-root=1; ws=1; same-site-strict=1; same-site-lax=1')
                return Promise.reject('Echoed cookie is incorrect: ' + cookie);

            postMessage("DONE");
        }
    ).catch(
        function (reason) {
            postMessage('FAIL: ' + reason);
        }
    );
}
