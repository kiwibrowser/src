importScripts('../../serviceworker/resources/worker-testharness.js');

// TODO(peter): Have a more generic exposed-interface-test, also pulling in
// the serviceworker-notification-event.js test.
test(function() {
    assert_false('requestPermission' in Notification);

}, 'Notification.requestPermission() is not exposed in Service Workers.');
