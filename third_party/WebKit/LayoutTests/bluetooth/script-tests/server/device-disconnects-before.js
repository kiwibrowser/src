'use strict';
bluetooth_test(t => {
  return setBluetoothFakeAdapter('DisconnectingHeartRateAdapter')
      .then(() => requestDeviceWithTrustedClick({
              filters: [{services: ['heart_rate']}],
              optionalServices: [request_disconnection_service_uuid]
            }))
      .then(device => {
        return device.gatt.connect()
            .then(gattServer => get_request_disconnection(gattServer))
            .then(requestDisconnection => requestDisconnection())
            .then(
                () => assert_promise_rejects_with_message(
                    device.gatt.CALLS(
                        [getPrimaryService('heart_rate') |
                         getPrimaryServices() |
                         getPrimaryServices('heart_rate')[UUID]]),
                    new DOMException(
                        'GATT Server is disconnected. ' +
                            'Cannot retrieve services. ' +
                            '(Re)connect first with `device.gatt.connect`.',
                        'NetworkError')));
      });
}, 'Device disconnects before FUNCTION_NAME. Reject with NetworkError.');
