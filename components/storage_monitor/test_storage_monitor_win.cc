// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TestStorageMonitorWin implementation.

#include "components/storage_monitor/test_storage_monitor_win.h"

#include "components/storage_monitor/test_portable_device_watcher_win.h"
#include "components/storage_monitor/test_volume_mount_watcher_win.h"

namespace storage_monitor {

TestStorageMonitorWin::TestStorageMonitorWin(
    TestVolumeMountWatcherWin* volume_mount_watcher,
    TestPortableDeviceWatcherWin* portable_device_watcher)
    : StorageMonitorWin(volume_mount_watcher, portable_device_watcher) {
  DCHECK(volume_mount_watcher_);
  DCHECK(portable_device_watcher);
}

TestStorageMonitorWin::~TestStorageMonitorWin() {
}

void TestStorageMonitorWin::InjectDeviceChange(UINT event_type, LPARAM data) {
  OnDeviceChange(event_type, data);
}

VolumeMountWatcherWin*
TestStorageMonitorWin::volume_mount_watcher() {
  return volume_mount_watcher_.get();
}

StorageMonitor::Receiver* TestStorageMonitorWin::receiver() const {
  return StorageMonitor::receiver();
}

}  // namespace storage_monitor
