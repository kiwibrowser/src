// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/device_event_router.h"

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/file_manager/volume_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace file_manager {
namespace {

namespace file_manager_private = extensions::api::file_manager_private;
typedef chromeos::disks::DiskMountManager::Disk Disk;

const char kTestDevicePath[] = "/device/test";

struct DeviceEvent {
  extensions::api::file_manager_private::DeviceEventType type;
  std::string device_path;
};

// DeviceEventRouter implementation for testing.
class DeviceEventRouterImpl : public DeviceEventRouter {
 public:
  DeviceEventRouterImpl()
      : DeviceEventRouter(base::TimeDelta::FromSeconds(0)),
        external_storage_disabled(false) {}
  ~DeviceEventRouterImpl() override {}

  // DeviceEventRouter overrides.
  void OnDeviceEvent(file_manager_private::DeviceEventType type,
                     const std::string& device_path) override {
    DeviceEvent event;
    event.type = type;
    event.device_path = device_path;
    events.push_back(event);
  }

  // DeviceEventRouter overrides.
  bool IsExternalStorageDisabled() override {
    return external_storage_disabled;
  }

  // List of dispatched events.
  std::vector<DeviceEvent> events;

  // Flag returned by |IsExternalStorageDisabled|.
  bool external_storage_disabled;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceEventRouterImpl);
};

}  // namespace

class DeviceEventRouterTest : public testing::Test {
 protected:
  void SetUp() override {
    device_event_router.reset(new DeviceEventRouterImpl());
  }

  // Creates a disk instance with |device_path| and |mount_path| for testing.
  Disk CreateTestDisk(const std::string& device_path,
                      const std::string& mount_path,
                      bool is_read_only_hardware) {
    return Disk(device_path, mount_path, false, "", "", "", "", "", "", "", "",
                "", device_path, chromeos::DEVICE_TYPE_UNKNOWN, 0, false,
                is_read_only_hardware, false, false, false, false, "vfat", "");
  }

  std::unique_ptr<DeviceEventRouterImpl> device_event_router;

 private:
  base::MessageLoop message_loop_;
};

TEST_F(DeviceEventRouterTest, AddAndRemoveDevice) {
  const Disk disk1 = CreateTestDisk("/device/test", "/mount/path1", false);
  const Disk disk1_unmounted = CreateTestDisk("/device/test", "", false);
  std::unique_ptr<Volume> volume(Volume::CreateForTesting(
      base::FilePath(FILE_PATH_LITERAL("/device/test")),
      base::FilePath(FILE_PATH_LITERAL("/mount/path1"))));
  device_event_router->OnDeviceAdded("/device/test");
  device_event_router->OnDiskAdded(disk1, true);
  device_event_router->OnVolumeMounted(chromeos::MOUNT_ERROR_NONE,
                                       *volume.get());
  device_event_router->OnVolumeUnmounted(chromeos::MOUNT_ERROR_NONE,
                                         *volume.get());
  device_event_router->OnDiskRemoved(disk1_unmounted);
  device_event_router->OnDeviceRemoved("/device/test");
  ASSERT_EQ(1u, device_event_router->events.size());
  EXPECT_EQ(file_manager_private::DEVICE_EVENT_TYPE_REMOVED,
            device_event_router->events[0].type);
  EXPECT_EQ("/device/test", device_event_router->events[0].device_path);
}

TEST_F(DeviceEventRouterTest, HardUnplugged) {
  const Disk disk1 = CreateTestDisk("/device/test", "/mount/path1", false);
  const Disk disk2 = CreateTestDisk("/device/test", "/mount/path2", false);
  device_event_router->OnDeviceAdded("/device/test");
  device_event_router->OnDiskAdded(disk1, true);
  device_event_router->OnDiskAdded(disk2, true);
  device_event_router->OnDiskRemoved(disk1);
  device_event_router->OnDiskRemoved(disk2);
  device_event_router->OnDeviceRemoved(kTestDevicePath);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(2u, device_event_router->events.size());
  EXPECT_EQ(file_manager_private::DEVICE_EVENT_TYPE_HARD_UNPLUGGED,
            device_event_router->events[0].type);
  EXPECT_EQ("/device/test", device_event_router->events[0].device_path);
  EXPECT_EQ(file_manager_private::DEVICE_EVENT_TYPE_REMOVED,
            device_event_router->events[1].type);
  EXPECT_EQ("/device/test", device_event_router->events[1].device_path);
}

TEST_F(DeviceEventRouterTest, HardUnplugReadOnlyVolume) {
  const Disk disk1 = CreateTestDisk("/device/test", "/mount/path1", true);
  const Disk disk2 = CreateTestDisk("/device/test", "/mount/path2", true);
  device_event_router->OnDeviceAdded("/device/test");
  device_event_router->OnDiskAdded(disk1, true);
  device_event_router->OnDiskAdded(disk2, true);
  device_event_router->OnDiskRemoved(disk1);
  device_event_router->OnDiskRemoved(disk2);
  device_event_router->OnDeviceRemoved(kTestDevicePath);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1u, device_event_router->events.size());
  EXPECT_EQ(file_manager_private::DEVICE_EVENT_TYPE_REMOVED,
            device_event_router->events[0].type);
  EXPECT_EQ("/device/test", device_event_router->events[0].device_path);
  // Should not warn hard unplug because the volumes are read-only.
}

}  // namespace file_manager
