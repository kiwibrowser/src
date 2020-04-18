// Supports test-runner control messages being send over |messagePort|, which enable
// workers to have limited access to TestRunner methods.
function supportTestRunnerMessagesOnPort(messagePort)
{
    if (!window.testRunner)
        return;

    messagePort.addEventListener('message', function(message) {
        if (message.data.type == 'simulateWebNotificationClick')
            testRunner.simulateWebNotificationClick(message.data.title);
    });
}

// Starts |script| as a dedicated worker and executes the testharness tests defined
// within it. The Notification-related testRunner methods will be made available.
function dedicatedWorkerTest(script)
{
    var worker = new Worker(script);
    supportTestRunnerMessagesOnPort(worker);

    fetch_tests_from_worker(worker);
}

// Starts |script| as a shared worker and executes the testharness tests defined
// within it. The Notification-related testRunner methods will be made available.
function sharedWorkerTest(script)
{
    var worker = new SharedWorker(script, 'Notification API LayoutTest worker');
    supportTestRunnerMessagesOnPort(worker.port);

    fetch_tests_from_worker(worker);
}

// Used by tests to announce that all tests have been registered when they're
// being ran from a dedicated or shared worker. Not necessary for document tests.
function isDedicatedOrSharedWorker()
{
    return false;
}

// Unregisters, then registers the Service Worker in |script| using |scope|, waits
// for it to activate and then inserts a message port. Returns a Promise which will
// be resolved with an object having the worker's registration and port. The
// Service Worker will be automatically unregistered once the test has completed.
function getActiveServiceWorkerWithMessagePort(test, script, scope)
{
    var registration = null;
    return service_worker_unregister_and_register(test, script, scope).then(function(swRegistration) {
        registration = swRegistration;
        add_completion_callback(function() {
            registration.unregister();
        });

        return wait_for_state(test, registration.installing, 'activated');
    }).then(function() {
        assert_not_equals(registration.active, null, 'The Service Worker needs to be activated.');
        return new Promise(function(resolve) {
            var messageChannel = new MessageChannel();
            messageChannel.port1.addEventListener('message', function(event) {
                if (event.data == 'ready')
                    resolve({ registration: registration, port: messageChannel.port1 });
            });

            registration.active.postMessage(messageChannel.port2, [messageChannel.port2]);
            messageChannel.port1.start();
        });
    });
}

// Sends a message with |data| over |port|. Returns a promise that either rejects when a bad
// response message is received, or otherwise resolves with the response data.
function sendCommand(port, data)
{
    return new Promise((resolve, reject) => {
        port.postMessage(data);
        port.addEventListener('message', function listener(event) {
            port.removeEventListener('message', listener);
            if (typeof event.data != 'object' || !event.data.command) {
                reject(new Error('Invalid message from the Service Worker.'));
            } else {
                resolve(event.data);
            }
        });
    });
}

// Simulates a click on the notification whose title equals |title|. The |actionIndex| specifies
// which action button to activate, if any.
function simulateNotificationClick(title, actionIndex, port)
{
    return new Promise((resolve, reject) => {
        testRunner.simulateWebNotificationClick(title, actionIndex);
        port.addEventListener('message', function listener(event) {
            port.removeEventListener('message', listener);
            if (typeof event.data != 'object' || event.data.command != 'click') {
                reject(new Error('Invalid message from the Service Worker.'));
            } else {
                resolve(event.data);
            }
        });
    });
}

// Sets up a listener for events of |type| on |target|. The listener is removed the first time the
// event fires. Returns a promise that resolves with the fired event.
function listenOnce(target, type)
{
    return new Promise((resolve, reject) => {
        target.addEventListener(type, function listener(event) {
            target.removeEventListener(type, listener);
            resolve(event);
        });
    });
}
