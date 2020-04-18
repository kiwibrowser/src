if (self.importScripts) {
    importScripts('/resources/testharness.js');
    importScripts('worker-helpers.js');
}

async_test(function(test) {
    if (Notification.permission != 'granted') {
        assert_unreached('No permission has been granted for displaying notifications.');
        return;
    }

    var notification = new Notification('My Notification');
    notification.addEventListener('show', function() {
        notification.close();
    });

    notification.addEventListener('close', function() {
        test.done();
    });

    notification.addEventListener('error', function() {
        assert_unreached('The error event should not be thrown.');
    });

}, 'Closing a notification should fire the onclose() event.');

if (isDedicatedOrSharedWorker())
    done();
