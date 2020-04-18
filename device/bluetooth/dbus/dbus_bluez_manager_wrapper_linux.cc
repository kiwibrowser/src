// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/dbus/dbus_bluez_manager_wrapper_linux.h"

#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/dbus_thread_manager_linux.h"

namespace bluez {

// static
void DBusBluezManagerWrapperLinux::Initialize() {
  DBusThreadManagerLinux::Initialize();
  BluezDBusManager::Initialize(
      bluez::DBusThreadManagerLinux::Get()->GetSystemBus(),
      false /* use_dbus_stub */);
}

// static
void DBusBluezManagerWrapperLinux::Shutdown() {
  bluez::BluezDBusManager::Shutdown();
  bluez::DBusThreadManagerLinux::Shutdown();
}

}  // namespace bluez
