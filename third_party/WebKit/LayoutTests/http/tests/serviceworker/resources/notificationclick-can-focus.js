// This helper will setup a small test framework that will use TESTS and run
// them sequentially and call self.postMessage('quit') when done.
// This helper also exposes |client|, |postMessage()|, |runNextTestOrQuit()|,
// |synthesizeNotificationClick()| and |initialize()|.
importScripts('sw-test-helpers.js');

var TESTS = [
    function testWithNoNotificationClick() {
        client.focus().catch(function() {
            self.postMessage('focus() outside of a notificationclick event failed');
        }).then(runNextTestOrQuit);
    },

    function testInStackOutOfWaitUntil() {
        synthesizeNotificationClick().then(function() {
            client.focus().then(function() {
                self.postMessage('focus() in notificationclick outside of waitUntil but in stack succeeded');
            }).then(runNextTestOrQuit);
        });
    },

    function testOutOfStackOutOfWaitUntil() {
        synthesizeNotificationClick().then(function() {
            self.clients.matchAll().then(function() {
                client.focus().catch(function() {
                    self.postMessage('focus() in notificationclick outside of waitUntil not in stack failed');
                }).then(runNextTestOrQuit);
            });
        });
    },

    function testInWaitUntilAsync() {
        synthesizeNotificationClick().then(function(e) {
            e.waitUntil(self.clients.matchAll().then(function() {
                return client.focus().then(function() {
                    self.postMessage('focus() in notificationclick\'s waitUntil suceeded');
                }).then(runNextTestOrQuit);
            }));
        });
    },

    function testDoubleCallInWaitUntilAsync() {
        synthesizeNotificationClick().then(function(e) {
            e.waitUntil(self.clients.matchAll().then(function() {
                return client.focus().then(function() {
                    return client.focus();
                }).catch(function() {
                    self.postMessage('focus() called twice failed');
                }).then(runNextTestOrQuit);
            }));
        });
    },

    function testWaitUntilTimeout() {
        var p = new Promise(function(resolve) {
            setTimeout(function() {
                resolve();
            }, 2000);
        });

        synthesizeNotificationClick().then(function(e) {
            e.waitUntil(p.then(function() {
                return client.focus().catch(function() {
                    self.postMessage('focus() failed after timeout');
                }).then(runNextTestOrQuit);
            }));
        });
    },

    function testFocusWindowOpenWindowCombo() {
        synthesizeNotificationClick().then(function(e) {
            e.waitUntil(clients.openWindow('/foo.html').then(function() {
                client.focus().catch(function() {
                    self.postMessage('focus() failed because a window was opened before');
                }).then(runNextTestOrQuit);
            }));
        });
    },
];

self.onmessage = function(e) {
    if (e.data == 'start') {
        e.waitUntil(initialize().then(runNextTestOrQuit));
    } else {
        e.waitUntil(initialize().then(function() {
            self.postMessage('received unexpected message');
            self.postMessage('quit');
        }));
    }
};
