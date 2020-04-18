// Allows a document to exercise the Push API within a service worker by sending commands.

// The port through which the document sends commands to the service worker.
var port = null;

// The most recently seen subscription.
var lastSeenSubscription = null;

self.addEventListener('message', function(workerEvent) {
    port = workerEvent.data;

    // Listen to incoming commands on the message port.
    port.onmessage = function(event) {
        if (typeof event.data != 'object' || !event.data.command)
            return;
        var options = event.data.options || { userVisibleOnly: true };
        switch (event.data.command) {
            case 'permissionState':
                self.registration.pushManager.permissionState(options).then(function(permissionStatus) {
                    port.postMessage({ command: event.data.command,
                                       success: true,
                                       permission: permissionStatus });
                }).catch(makeErrorHandler(event.data.command));
                break;

            case 'subscribe':
                self.registration.pushManager.subscribe(options).then(function(subscription) {
                    lastSeenSubscription = subscription;
                    port.postMessage({ command: event.data.command,
                                       success: true,
                                       endpoint: subscription.endpoint });
                }).catch(makeErrorHandler(event.data.command));
                break;

            case 'getSubscription':
                self.registration.pushManager.getSubscription().then(function(subscription) {
                    lastSeenSubscription = subscription;
                    var endpoint = subscription ? subscription.endpoint : null;
                    port.postMessage({ command: event.data.command,
                                       success: true,
                                       endpoint: endpoint });
                }).catch(makeErrorHandler(event.data.command));
                break;

            case 'unsubscribe':
                self.registration.pushManager.getSubscription()
                    .then(function(subscription) {
                        // We keep track of lastSeenSubscription so we can attempt to unsubscribe
                        // more than once.
                        subscription = subscription || lastSeenSubscription;
                        if (!subscription)
                            throw new Error('There is no subscription to unsubscribe');
                        return subscription.unsubscribe();
                    })
                    .then(function(unsubscribeResult) {
                        port.postMessage({ command: event.data.command,
                                           success: true,
                                           unsubscribeResult: unsubscribeResult });
                    })
                    .catch(makeErrorHandler(event.data.command));
                break;

            default:
                port.postMessage({ command: 'error',
                                   errorMessage: 'Invalid command: ' + event.data.command });
                break;
        }
    };

    // Notify the controller that the worker is now available.
    port.postMessage('ready');
});

function makeErrorHandler(command) {
    return function(error) {
        var errorMessage = error ? error.message : 'unknown error';
        port.postMessage({ command: command,
                           success: false,
                           errorMessage: errorMessage });
    };
}
