# WebUSB Blink Module

`Source/modules/webusb` implements the renderer process details and bindings
for the [WebUSB specification]. It communicates with the browser process through the [public Mojo interface] of `//device/usb` to the [UsbService].

[WebUSB specification]: https://wicg.github.io/webusb/
[public Mojo interface]: /device/usb/public/mojom
[UsbService]: /device/usb/usb_service.h


## Testing

WebUSB is primarily tested in [Web Platform Tests].
Chromium implementation details are tested in [Layout Tests].

[Web Platform Tests]: ../../../LayoutTests/external/wpt/webusb/
[Layout Tests]: ../../../LayoutTests/usb/
