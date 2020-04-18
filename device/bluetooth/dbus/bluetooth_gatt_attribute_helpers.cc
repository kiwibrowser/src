// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/dbus/bluetooth_gatt_attribute_helpers.h"

#include <string>

#include "base/logging.h"
#include "dbus/message.h"
#include "dbus/object_path.h"

namespace bluez {

namespace {

constexpr char kDeviceField[] = "device";

}  // namespace

dbus::ObjectPath ReadDevicePath(dbus::MessageReader* reader) {
  dbus::MessageReader array_reader(nullptr);
  if (!reader->PopArray(&array_reader))
    return dbus::ObjectPath();

  // Go through all the keys in the dictionary to find the device key.
  while (array_reader.HasMoreData()) {
    dbus::MessageReader dict_entry_reader(nullptr);
    std::string key;
    if (!array_reader.PopDictEntry(&dict_entry_reader) ||
        !dict_entry_reader.PopString(&key)) {
      return dbus::ObjectPath();
    } else if (key == kDeviceField) {
      dbus::ObjectPath device_path;
      dict_entry_reader.PopVariantOfObjectPath(&device_path);
      return device_path;
    }
  }

  return dbus::ObjectPath();
}

}  // namespace bluez
