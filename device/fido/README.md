# FIDO

`//device/fido` contains abstractions for [FIDO](https://fidoalliance.org/)
security keys across multiple platforms.

## U2F Security Keys

Support for [U2F (FIDO
1.2)](https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-overview-v1.2-ps-20170411.html)
security keys is present for both USB Human Interface Devices (USB HID) and
Bluetooth Low Energy (BLE) devices. Clients can perform U2F operations using the
[`U2fRegister`](u2f_register.h) and [`U2fSign`](u2f_sign.h) classes. These
abstractions automatically perform device discovery and handle communication
with the underlying devices. Talking to HID devices is done using the [HID Mojo
service](/services/device/public/interfaces/hid.mojom), while communication with
BLE devices is done using abstractions found in
[`//device/bluetooth`](/device/bluetooth/). HID is supported on all desktop
platforms, while BLE lacks some support on Windows (see
[`//device/bluetooth/README.md`](/device/bluetooth/README.md) for details).

## CTAP Security Keys

Support for [CTAP2 (FIDO
2.0)](https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html)
security keys is in active development and aims to unify the implementations for
both U2F and CTAP keys.

## Testing

### Unit Tests

Standard use of `*_unittest.cc` files for must code coverage. Files prefixed
with `mock_` provide GoogleMock based fake objects for easy mocking of
dependencies during testing.

### Fuzzers

[libFuzzer] tests are in `*_fuzzer.cc` files. They test for bad input from
devices, e.g. when parsing responses to register or sign operations.

[libFuzzer]: /testing/libfuzzer/README.md
