// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/dbus/dbus_thread_manager_linux.h"

#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "dbus/bus.h"

namespace bluez {

static DBusThreadManagerLinux* g_linux_dbus_manager = NULL;

DBusThreadManagerLinux::DBusThreadManagerLinux() {
  base::Thread::Options thread_options;
  thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
  dbus_thread_.reset(new base::Thread("D-Bus thread"));
  dbus_thread_->StartWithOptions(thread_options);

  // Create the connection to the system bus.
  dbus::Bus::Options system_bus_options;
  system_bus_options.bus_type = dbus::Bus::SYSTEM;
  system_bus_options.connection_type = dbus::Bus::PRIVATE;
  system_bus_options.dbus_task_runner = dbus_thread_->task_runner();
  system_bus_ = new dbus::Bus(system_bus_options);
}

DBusThreadManagerLinux::~DBusThreadManagerLinux() {
  // Shut down the bus. During the browser shutdown, it's ok to shut down
  // the bus synchronously.
  if (system_bus_.get())
    system_bus_->ShutdownOnDBusThreadAndBlock();

  // Stop the D-Bus thread.
  if (dbus_thread_)
    dbus_thread_->Stop();

  if (!g_linux_dbus_manager)
    return;  // Called form Shutdown() or local test instance.

  // There should never be both a global instance and a local instance.
  CHECK(this == g_linux_dbus_manager);
}

dbus::Bus* DBusThreadManagerLinux::GetSystemBus() {
  return system_bus_.get();
}

// static
void DBusThreadManagerLinux::Initialize() {
  CHECK(!g_linux_dbus_manager);
  g_linux_dbus_manager = new DBusThreadManagerLinux();
}

// static
void DBusThreadManagerLinux::Shutdown() {
  // Ensure that we only shutdown LinuxDBusManager once.
  CHECK(g_linux_dbus_manager);
  DBusThreadManagerLinux* dbus_thread_manager = g_linux_dbus_manager;
  g_linux_dbus_manager = NULL;
  delete dbus_thread_manager;
  VLOG(1) << "LinuxDBusManager Shutdown completed";
}

// static
DBusThreadManagerLinux* DBusThreadManagerLinux::Get() {
  CHECK(g_linux_dbus_manager)
      << "LinuxDBusManager::Get() called before Initialize()";
  return g_linux_dbus_manager;
}

}  // namespace bluez
