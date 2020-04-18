// Tests that the notification available after the given operation is executed
// accurately reflects the data attributes of several types with which the
// notification was created in the document.
function runNotificationDataReflectionTest(test, notificationOperation) {
    var scope = 'resources/scope/' + location.pathname,
        script = 'instrumentation-service-worker.js';

    // Set notification's data of several types to a structured clone of options's data.
    var notificationDataList = new Array(
        true, // Check Boolean type
        1024, // Check Number type
        Number.NaN, // Check Number.NaN type
        'any data', // Check String type
        null, // Check null
        new Array('Saab', 'Volvo', 'BMW'),  // Check Array type
        { first: 'first', second: 'second' }  // Check object
    );

    PermissionsHelper.setPermission('notifications', 'granted').then(function() {
        getActiveServiceWorkerWithMessagePort(test, script, scope).then(function(workerInfo) {
            // (1) Tell the Service Worker to display a Web Notification.
            var assertNotificationDataReflects = function(pos) {
                workerInfo.port.postMessage({
                    command: 'show',

                    title: scope,
                    options: {
                        title: scope,
                        tag: pos,
                        data: notificationDataList[pos]
                    }
                });
            };

            workerInfo.port.addEventListener('message', function(event) {
                if (typeof event.data != 'object' || !event.data.command) {
                    assert_unreached('Invalid message from the Service Worker.');
                    return;
                }

                // (2) Listen for confirmation from the Service Worker that the
                // notification's display promise has been resolved.
                if (event.data.command == 'show') {
                    assert_true(event.data.success, 'The notification must have been displayed.');
                    notificationOperation.run(scope);
                    return;
                }

                // (3) Listen for confirmation from the Service Worker that the
                // notification has been closed. Make sure that all properties
                // set on the Notification object are as expected.
                assert_equals(event.data.command, notificationOperation.name, 'Notification was expected to receive different operation');

                var pos = event.data.notification.tag;

                if (typeof notificationDataList[pos] === 'object' && notificationDataList[pos] !== null)
                    assert_object_equals(event.data.notification.data, notificationDataList[pos], 'The data field must be the same.');
                else
                    assert_equals(event.data.notification.data, notificationDataList[pos], 'The data field must be the same.');

                if (++pos < notificationDataList.length)
                    assertNotificationDataReflects(pos);
                else
                    test.done();
            });

            assertNotificationDataReflects(0);
        }).catch(unreached_rejection(test));
    });
}
