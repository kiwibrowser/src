# Web Bluetooth Service in Content

`content/*/bluetooth` implements the [Web Bluetooth specification]
using the [/device/bluetooth] code module.

This service is exposed to the web in the [blink bluetooth module].

[Web Bluetooth specification]: https://webbluetoothcg.github.io/web-bluetooth/
[/device/bluetooth]: /device/bluetooth
[blink bluetooth module]: /third_party/blink/renderer/modules/bluetooth/


## Testing

Bluetooth layout tests in `third_party/WebKit/LayoutTests/bluetooth/` rely on
fake Bluetooth implementation classes constructed in
`content/shell/browser/layout_test/layout_test_bluetooth_adapter_provider`.
These tests span JavaScript binding to the `device/bluetooth` API layer.


## Design Documents

See: [Class Diagram of Web Bluetooth through Bluetooth Android][Class]

[Class]: https://sites.google.com/a/chromium.org/dev/developers/design-documents/bluetooth-design-docs/web-bluetooth-through-bluetooth-android-class-diagram

