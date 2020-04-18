'use strict';
const test_desc = 'Device disconnects before FUNCTION_NAME. ' +
    'Reject with NetworkError.';
const expected = new DOMException('GATT Server is disconnected. Cannot ' +
    'perform GATT operations. (Re)connect first with `device.gatt.connect`.',
    'NetworkError')
let device, characteristic, fake_peripheral;

bluetooth_test(() => getMeasurementIntervalCharacteristic()
    .then(_ => ({device, characteristic, fake_peripheral} = _))
    .then(() => simulateGATTDisconnectionAndWait(device, fake_peripheral))
    .then(() => assert_promise_rejects_with_message(
          characteristic.CALLS([
            readValue()|
            writeValue(new Uint8Array(1 /*length */))|
            startNotifications()|
            stopNotifications()
          ]),
          expected)),
    test_desc);
