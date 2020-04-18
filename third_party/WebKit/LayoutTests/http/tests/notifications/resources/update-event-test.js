if (self.importScripts) {
    importScripts('/resources/testharness.js');
    importScripts('worker-helpers.js');
}

async_test(function(test) {
    if (Notification.permission != 'granted') {
        assert_unreached('No permission has been granted for displaying notifications.');
        return;
    }

    // We require two asynchronous events to happen when a notification gets updated, (1)
    // the old instance should receive the "close" event, and (2) the new notification
    // should receive the "show" event, but only after (1) has happened.
    var closedOriginalNotification = false;

    var notification = new Notification('My Notification', { tag: 'notification-test' });
    notification.addEventListener('show', function() {
        var updatedNotification = new Notification('Second Notification', { tag: 'notification-test' });
        updatedNotification.addEventListener('show', function() {
            assert_true(closedOriginalNotification);
            test.done();
        });
    });

    notification.addEventListener('close', function() {
        closedOriginalNotification = true;
    });

    notification.addEventListener('error', function() {
        assert_unreached('The error event should not be thrown.');
    });

}, 'Replacing a notification will discard the previous notification.');

if (isDedicatedOrSharedWorker())
    done();
