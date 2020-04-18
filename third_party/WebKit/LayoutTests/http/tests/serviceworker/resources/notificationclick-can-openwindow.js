// This helper will setup a small test framework that will use TESTS and run
// them sequentially and call self.postMessage('quit') when done.
// This helper also exposes |client|, |postMessage()|, |runNextTestOrQuit()|,
// |synthesizeNotificationClick()| and |initialize()|.
importScripts('sw-test-helpers.js');

var TESTS = [
    function testWithNoNotificationClick() {
        clients.openWindow('/foo.html').catch(function() {
            self.postMessage('openWindow() outside of a notificationclick event failed');
        }).then(runNextTestOrQuit);
    },

    function testInStackOutOfWaitUntil() {
        synthesizeNotificationClick().then(function() {
            clients.openWindow('/foo.html').then(function() {
                self.postMessage('openWindow() in notificationclick outside of waitUntil but in stack succeeded');
            }).then(runNextTestOrQuit);
        });
    },

    function testOutOfStackOutOfWaitUntil() {
        synthesizeNotificationClick().then(function() {
            self.clients.matchAll().then(function() {
                clients.openWindow('/foo.html').catch(function() {
                    self.postMessage('openWindow() in notificationclick outside of waitUntil not in stack failed');
                }).then(runNextTestOrQuit);
            });
        });
    },

    function testInWaitUntilAsyncAndDoubleCall() {
        synthesizeNotificationClick().then(function(e) {
            e.waitUntil(self.clients.matchAll().then(function() {
                return clients.openWindow('/foo.html').then(function() {
                    self.postMessage('openWindow() in notificationclick\'s waitUntil suceeded');
                }).then(runNextTestOrQuit);
            }));
        });
    },

    function testDoubleCallInWaitUntilAsync() {
        synthesizeNotificationClick().then(function(e) {
            e.waitUntil(self.clients.matchAll().then(function() {
                return clients.openWindow('/foo.html').then(function() {
                    return clients.openWindow('/foo.html');
                }).catch(function() {
                    self.postMessage('openWindow() called twice failed');
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
                return clients.openWindow('/foo.html').catch(function() {
                    self.postMessage('openWindow() failed after timeout');
                }).then(runNextTestOrQuit);
            }));
        });
    },

    function testFocusWindowOpenWindowCombo() {
        synthesizeNotificationClick().then(function(e) {
            e.waitUntil(client.focus().then(function() {
                clients.openWindow().catch(function() {
                    self.postMessage('openWindow() failed because a window was focused before');
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
