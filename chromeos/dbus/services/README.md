This directory contains service files and implementations for D-Bus
services provided by Chrome.

Service files (with `.conf` extensions) must be listed in the
`dbus_service_files` rule of `chromeos/BUILD.gn` so that they can be
deployed in Chrome OS and [Simple Chrome] builds.

[Simple Chrome]: https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/building-chromium-browser
