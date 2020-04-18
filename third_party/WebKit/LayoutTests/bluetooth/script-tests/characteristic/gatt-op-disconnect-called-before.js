'use strict';
const test_desc = 'disconnect() called before FUNCTION_NAME. ' +
    'Reject with NetworkError.';
const value = new Uint8Array([1]);
const expected = new DOMException('GATT Server is disconnected. Cannot ' +
    'perform GATT operations. (Re)connect first with `device.gatt.connect`.',
    'NetworkError')
let device, characteristic, fake_peripheral;

bluetooth_test(() => getMeasurementIntervalCharacteristic()
    .then(_ => ({device, characteristic, fake_peripheral} = _))
    .then(() => {
      device.gatt.disconnect();
      return assert_promise_rejects_with_message(
        characteristic.CALLS([
          readValue()|
          writeValue(value)|
          startNotifications()|
          stopNotifications()
        ]),
        expected);
    }),
    test_desc);
