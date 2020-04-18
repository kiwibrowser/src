'use strict';
const test_desc = 'Device disconnects before FUNCTION_NAME. ' +
    'Reject with NetworkError.';
const expected = new DOMException(
    'GATT Server is disconnected. Cannot retrieve descriptors. (Re)connect ' +
    'first with `device.gatt.connect`.',
    'NetworkError');
let device, characteristic, fake_peripheral;

bluetooth_test(() => getMeasurementIntervalCharacteristic()
    .then(_ =>  ({device, characteristic, fake_peripheral} = _))
    .then(() => simulateGATTDisconnectionAndWait(device, fake_peripheral))
    .then(() => assert_promise_rejects_with_message(
        characteristic.CALLS([
          getDescriptor(user_description.name) |
          getDescriptors(user_description.name)[UUID] |
          getDescriptors()
        ]),
        expected)),
    test_desc);
