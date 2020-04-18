// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains a subclass of PortableDeviceWatcherWin to expose some
// functionality for testing.

#ifndef COMPONENTS_STORAGE_MONITOR_TEST_PORTABLE_DEVICE_WATCHER_WIN_H_
#define COMPONENTS_STORAGE_MONITOR_TEST_PORTABLE_DEVICE_WATCHER_WIN_H_

#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/storage_monitor/portable_device_watcher_win.h"

namespace storage_monitor {

class TestPortableDeviceWatcherWin : public PortableDeviceWatcherWin {
 public:
  // MTP device PnP identifiers.
  static const base::char16 kMTPDeviceWithMultipleStorages[];
  static const base::char16 kMTPDeviceWithInvalidInfo[];
  static const base::char16 kMTPDeviceWithValidInfo[];

  // MTP device storage unique identifier.
  static const char kStorageUniqueIdA[];

  TestPortableDeviceWatcherWin();
  ~TestPortableDeviceWatcherWin() override;

  // Returns the persistent storage unique id of the device specified by the
  // |pnp_device_id|. |storage_object_id| specifies the string ID that uniquely
  // identifies the object on the device.
  static std::string GetMTPStorageUniqueId(
      const base::string16& pnp_device_id,
      const base::string16& storage_object_id);

  // Returns a list of storage object identifiers of the media transfer protocol
  // (MTP) device given a |pnp_device_id|.
  static PortableDeviceWatcherWin::StorageObjectIDs GetMTPStorageObjectIds(
      const base::string16& pnp_device_id);

  // Gets the media transfer protocol (MTP) device storage details given a
  // |pnp_device_id| and |storage_object_id|.
  static void GetMTPStorageDetails(const base::string16& pnp_device_id,
                                   const base::string16& storage_object_id,
                                   base::string16* device_location,
                                   std::string* unique_id,
                                   base::string16* name);

  // Returns a list of device storage details for the given device specified by
  // |pnp_device_id|.
  static PortableDeviceWatcherWin::StorageObjects GetDeviceStorageObjects(
      const base::string16& pnp_device_id);

  // Used by MediaFileSystemRegistry unit test.
  void set_use_dummy_mtp_storage_info(bool use_dummy_info) {
    use_dummy_mtp_storage_info_ = use_dummy_info;
  }

 private:
  // PortableDeviceWatcherWin:
  void EnumerateAttachedDevices() override;
  void HandleDeviceAttachEvent(const base::string16& pnp_device_id) override;
  bool GetMTPStorageInfoFromDeviceId(
      const std::string& storage_device_id,
      base::string16* device_location,
      base::string16* storage_object_id) const override;

  // Set to true to get dummy storage details from
  // GetMTPStorageInfoFromDeviceId().
  bool use_dummy_mtp_storage_info_;

  DISALLOW_COPY_AND_ASSIGN(TestPortableDeviceWatcherWin);
};

}  // namespace storage_monitor

#endif  // COMPONENTS_STORAGE_MONITOR_TEST_PORTABLE_DEVICE_WATCHER_WIN_H_
