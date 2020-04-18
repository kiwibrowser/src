# Device Event Log

This directory contains code for logging device and system events.

## Usage

Use device event log macros to record events without contributing to noise in
the chrome log.

* Events are stored in a circular buffer (current limit is 4000).
* Events can be viewed at chrome://device-log. Events can be filtered by type
  and level.
* Events show up in **feedback reports** under `device_event_log`.
* Network events are separated out into a `network_event_log` section.
* **ERROR** events will also be logged to the main chrome log.
* All events can be logged to the main chrome log using vlog:
  `--vmodule=device_event_log*=1`

The events can also be queried for viewing in other informational pages, e.g:
```
device_event_log::GetAsString(device_event_log::OLDEST_FIRST, "json",
                              "bluetooth", device_event_log::LOG_LEVEL_DEBUG,
                              1000);
```

## Examples

Typical usage:

```NET_LOG(EVENT) << "NetworkState Changed " << name << ": " << state;```

```POWER_LOG(USER) << "Suspend requested";```

```POWER_LOG(DEBUG) << "Sending suspend request to dbus object: " << path;```

```BLUETOOTH_LOG(ERROR) << "Unrecognized DBus error " << error_name;```

Advanced usage:

```
device_event_log::LogLevel log_level =
      SuppressError(dbus_error_message) ? device_event_log::LOG_LEVEL_DEBUG
                                        : device_event_log::LOG_LEVEL_ERROR;
DEVICE_LOG(device_event_log::LOG_TYPE_NETWORK, log_level) << detail;
```

```
USB_PLOG(DEBUG) << "Failed to set configuration " << configuration_value;
```
