// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/extensions/file_manager/event_router.h"
#include "chrome/browser/chromeos/file_manager/file_watcher.h"
#include "chrome/browser/chromeos/file_manager/mount_test_util.h"
#include "chrome/browser/chromeos/file_system_provider/icon_set.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/extensions/api/file_system_provider_capabilities/file_system_provider_capabilities_handler.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/dbus/cros_disks_client.h"
#include "chromeos/disks/mock_disk_mount_manager.h"
#include "components/drive/file_change.h"
#include "extensions/common/extension.h"
#include "extensions/common/install_warning.h"
#include "google_apis/drive/test_util.h"
#include "storage/browser/fileapi/external_mount_points.h"

using ::testing::_;
using ::testing::ReturnRef;

using chromeos::disks::DiskMountManager;

namespace {

struct TestDiskInfo {
  const char* system_path;
  const char* file_path;
  bool write_disabled_by_policy;
  const char* device_label;
  const char* drive_label;
  const char* vendor_id;
  const char* vendor_name;
  const char* product_id;
  const char* product_name;
  const char* fs_uuid;
  const char* system_path_prefix;
  chromeos::DeviceType device_type;
  uint64_t size_in_bytes;
  bool is_parent;
  bool is_read_only_hardware;
  bool has_media;
  bool on_boot_device;
  bool on_removable_device;
  bool is_hidden;
  const char* file_system_type;
  const char* base_mount_path;
};

struct TestMountPoint {
  std::string source_path;
  std::string mount_path;
  chromeos::MountType mount_type;
  chromeos::disks::MountCondition mount_condition;

  // -1 if there is no disk info.
  int disk_info_index;
};

TestDiskInfo kTestDisks[] = {{"system_path1",
                              "file_path1",
                              false,
                              "device_label1",
                              "drive_label1",
                              "0123",
                              "vendor1",
                              "abcd",
                              "product1",
                              "FFFF-FFFF",
                              "system_path_prefix1",
                              chromeos::DEVICE_TYPE_USB,
                              1073741824,
                              false,
                              false,
                              false,
                              false,
                              false,
                              false,
                              "exfat",
                              ""},
                             {"system_path2",
                              "file_path2",
                              false,
                              "device_label2",
                              "drive_label2",
                              "4567",
                              "vendor2",
                              "cdef",
                              "product2",
                              "0FFF-FFFF",
                              "system_path_prefix2",
                              chromeos::DEVICE_TYPE_MOBILE,
                              47723,
                              true,
                              true,
                              true,
                              true,
                              false,
                              false,
                              "exfat",
                              ""},
                             {"system_path3",
                              "file_path3",
                              true,  // write_disabled_by_policy
                              "device_label3",
                              "drive_label3",
                              "89ab",
                              "vendor3",
                              "ef01",
                              "product3",
                              "00FF-FFFF",
                              "system_path_prefix3",
                              chromeos::DEVICE_TYPE_OPTICAL_DISC,
                              0,
                              true,
                              false,  // is_hardware_read_only
                              false,
                              true,
                              false,
                              false,
                              "exfat",
                              ""}};

void DispatchDirectoryChangeEventImpl(
    int* counter,
    const base::FilePath& virtual_path,
    const drive::FileChange* list,
    bool got_error,
    const std::vector<std::string>& extension_ids) {
  ++(*counter);
}

void AddFileWatchCallback(bool success) {}

bool InitializeLocalFileSystem(std::string mount_point_name,
                               base::ScopedTempDir* temp_dir,
                               base::FilePath* mount_point_dir) {
  const char kTestFileContent[] = "The five boxing wizards jumped quickly";
  if (!temp_dir->CreateUniqueTempDir())
    return false;

  *mount_point_dir = temp_dir->GetPath().AppendASCII(mount_point_name);
  // Create the mount point.
  if (!base::CreateDirectory(*mount_point_dir))
    return false;

  const base::FilePath test_dir = mount_point_dir->AppendASCII("test_dir");
  if (!base::CreateDirectory(test_dir))
    return false;

  const base::FilePath test_file = test_dir.AppendASCII("test_file.txt");
  if (!google_apis::test_util::WriteStringToFile(test_file, kTestFileContent))
    return false;

  return true;
}

}  // namespace

class FileManagerPrivateApiTest : public extensions::ExtensionApiTest {
 public:
  FileManagerPrivateApiTest() : disk_mount_manager_mock_(nullptr) {
    InitMountPoints();
  }

  ~FileManagerPrivateApiTest() override {
    DCHECK(!disk_mount_manager_mock_);
    DCHECK(!testing_profile_);
    DCHECK(!event_router_);
  }

  void SetUpOnMainThread() override {
    extensions::ExtensionApiTest::SetUpOnMainThread();

    testing_profile_.reset(new TestingProfile());
    event_router_.reset(new file_manager::EventRouter(testing_profile_.get()));
  }

  void TearDownOnMainThread() override {
    event_router_->Shutdown();

    event_router_.reset();
    testing_profile_.reset();

    extensions::ExtensionApiTest::TearDownOnMainThread();
  }

  // ExtensionApiTest override
  void SetUpInProcessBrowserTestFixture() override {
    extensions::ExtensionApiTest::SetUpInProcessBrowserTestFixture();
    disk_mount_manager_mock_ = new chromeos::disks::MockDiskMountManager;
    chromeos::disks::DiskMountManager::InitializeForTesting(
        disk_mount_manager_mock_);
    disk_mount_manager_mock_->SetupDefaultReplies();

    // override mock functions.
    ON_CALL(*disk_mount_manager_mock_, FindDiskBySourcePath(_)).WillByDefault(
        Invoke(this, &FileManagerPrivateApiTest::FindVolumeBySourcePath));
    EXPECT_CALL(*disk_mount_manager_mock_, disks())
        .WillRepeatedly(ReturnRef(volumes_));
    EXPECT_CALL(*disk_mount_manager_mock_, mount_points())
        .WillRepeatedly(ReturnRef(mount_points_));
  }

  // ExtensionApiTest override
  void TearDownInProcessBrowserTestFixture() override {
    chromeos::disks::DiskMountManager::Shutdown();
    disk_mount_manager_mock_ = nullptr;

    extensions::ExtensionApiTest::TearDownInProcessBrowserTestFixture();
  }

 private:
  void InitMountPoints() {
    const TestMountPoint kTestMountPoints[] = {
      {
        "device_path1",
        chromeos::CrosDisksClient::GetRemovableDiskMountPoint().AppendASCII(
            "mount_path1").AsUTF8Unsafe(),
        chromeos::MOUNT_TYPE_DEVICE,
        chromeos::disks::MOUNT_CONDITION_NONE,
        0
      },
      {
        "device_path2",
        chromeos::CrosDisksClient::GetRemovableDiskMountPoint().AppendASCII(
            "mount_path2").AsUTF8Unsafe(),
        chromeos::MOUNT_TYPE_DEVICE,
        chromeos::disks::MOUNT_CONDITION_NONE,
        1
      },
      {
        "device_path3",
        chromeos::CrosDisksClient::GetRemovableDiskMountPoint().AppendASCII(
            "mount_path3").AsUTF8Unsafe(),
        chromeos::MOUNT_TYPE_DEVICE,
        chromeos::disks::MOUNT_CONDITION_NONE,
        2
      },
      {
        // Set source path inside another mounted volume.
        chromeos::CrosDisksClient::GetRemovableDiskMountPoint().AppendASCII(
            "mount_path3/archive.zip").AsUTF8Unsafe(),
        chromeos::CrosDisksClient::GetArchiveMountPoint().AppendASCII(
            "archive_mount_path").AsUTF8Unsafe(),
        chromeos::MOUNT_TYPE_ARCHIVE,
        chromeos::disks::MOUNT_CONDITION_NONE,
        -1
      }
    };

    for (size_t i = 0; i < arraysize(kTestMountPoints); i++) {
      mount_points_.insert(DiskMountManager::MountPointMap::value_type(
          kTestMountPoints[i].mount_path,
          DiskMountManager::MountPointInfo(kTestMountPoints[i].source_path,
                                           kTestMountPoints[i].mount_path,
                                           kTestMountPoints[i].mount_type,
                                           kTestMountPoints[i].mount_condition)
      ));
      int disk_info_index = kTestMountPoints[i].disk_info_index;
      if (kTestMountPoints[i].disk_info_index >= 0) {
        EXPECT_GT(arraysize(kTestDisks), static_cast<size_t>(disk_info_index));
        if (static_cast<size_t>(disk_info_index) >= arraysize(kTestDisks))
          return;

        volumes_.insert(DiskMountManager::DiskMap::value_type(
            kTestMountPoints[i].source_path,
            std::make_unique<DiskMountManager::Disk>(
                kTestMountPoints[i].source_path, kTestMountPoints[i].mount_path,
                kTestDisks[disk_info_index].write_disabled_by_policy,
                kTestDisks[disk_info_index].system_path,
                kTestDisks[disk_info_index].file_path,
                kTestDisks[disk_info_index].device_label,
                kTestDisks[disk_info_index].drive_label,
                kTestDisks[disk_info_index].vendor_id,
                kTestDisks[disk_info_index].vendor_name,
                kTestDisks[disk_info_index].product_id,
                kTestDisks[disk_info_index].product_name,
                kTestDisks[disk_info_index].fs_uuid,
                kTestDisks[disk_info_index].system_path_prefix,
                kTestDisks[disk_info_index].device_type,
                kTestDisks[disk_info_index].size_in_bytes,
                kTestDisks[disk_info_index].is_parent,
                kTestDisks[disk_info_index].is_read_only_hardware,
                kTestDisks[disk_info_index].has_media,
                kTestDisks[disk_info_index].on_boot_device,
                kTestDisks[disk_info_index].on_removable_device,
                kTestDisks[disk_info_index].is_hidden,
                kTestDisks[disk_info_index].file_system_type,
                kTestDisks[disk_info_index].base_mount_path)));
      }
    }
  }

  const DiskMountManager::Disk* FindVolumeBySourcePath(
      const std::string& source_path) {
    auto volume_it = volumes_.find(source_path);
    return (volume_it == volumes_.end()) ? nullptr : volume_it->second.get();
  }

 protected:
  chromeos::disks::MockDiskMountManager* disk_mount_manager_mock_;
  DiskMountManager::DiskMap volumes_;
  DiskMountManager::MountPointMap mount_points_;
  std::unique_ptr<TestingProfile> testing_profile_;
  std::unique_ptr<file_manager::EventRouter> event_router_;
};

IN_PROC_BROWSER_TEST_F(FileManagerPrivateApiTest, Mount) {
  using chromeos::file_system_provider::IconSet;
  file_manager::test_util::WaitUntilDriveMountPointIsAdded(
      browser()->profile());

  // Add a provided file system, to test passing the |configurable| and
  // |source| flags properly down to Files app.
  IconSet icon_set;
  icon_set.SetIcon(IconSet::IconSize::SIZE_16x16,
                   GURL("chrome://resources/testing-provider-id-16.jpg"));
  icon_set.SetIcon(IconSet::IconSize::SIZE_32x32,
                   GURL("chrome://resources/testing-provider-id-32.jpg"));
  chromeos::file_system_provider::ProvidedFileSystemInfo info(
      "testing-provider-id", chromeos::file_system_provider::MountOptions(),
      base::FilePath(), true /* configurable */, false /* watchable */,
      extensions::SOURCE_NETWORK, icon_set);

  file_manager::VolumeManager::Get(browser()->profile())
      ->AddVolumeForTesting(file_manager::Volume::CreateForProvidedFileSystem(
          info, file_manager::MOUNT_CONTEXT_AUTO));

  // We will call fileManagerPrivate.unmountVolume once. To test that method, we
  // check that UnmountPath is really called with the same value.
  EXPECT_CALL(*disk_mount_manager_mock_, UnmountPath(_, _, _))
      .Times(0);
  EXPECT_CALL(*disk_mount_manager_mock_,
              UnmountPath(
                  chromeos::CrosDisksClient::GetArchiveMountPoint().AppendASCII(
                      "archive_mount_path").AsUTF8Unsafe(),
                  chromeos::UNMOUNT_OPTIONS_NONE, _)).Times(1);

  ASSERT_TRUE(RunComponentExtensionTest("file_browser/mount_test"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(FileManagerPrivateApiTest, Permissions) {
  EXPECT_TRUE(
      RunExtensionTestIgnoreManifestWarnings("file_browser/permissions"));
  const extensions::Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension);
  ASSERT_EQ(1u, extension->install_warnings().size());
  const extensions::InstallWarning& warning = extension->install_warnings()[0];
  EXPECT_EQ("fileManagerPrivate", warning.key);
}

IN_PROC_BROWSER_TEST_F(FileManagerPrivateApiTest, OnFileChanged) {
  // In drive volume, deletion of a directory is notified via OnFileChanged.
  // Local changes directly come to HandleFileWatchNotification from
  // FileWatcher.
  typedef drive::FileChange FileChange;
  typedef drive::FileChange::FileType FileType;
  typedef drive::FileChange::ChangeType ChangeType;

  int counter = 0;
  event_router_->SetDispatchDirectoryChangeEventImplForTesting(
      base::Bind(&DispatchDirectoryChangeEventImpl, &counter));

  // /a/b/c and /a/d/e are being watched.
  event_router_->AddFileWatch(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/a/b/c")),
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs-virtual/root/a/b/c")),
      "extension_1", base::Bind(&AddFileWatchCallback));

  event_router_->AddFileWatch(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/a/d/e")),
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs-hash/root/a/d/e")),
      "extension_2", base::Bind(&AddFileWatchCallback));

  event_router_->AddFileWatch(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/aaa")),
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs-hash/root/aaa")),
      "extension_3", base::Bind(&AddFileWatchCallback));

  // event_router->addFileWatch create some tasks which are performed on
  // TaskScheduler. Wait until they are done.
  base::TaskScheduler::GetInstance()->FlushForTesting();
  // We also wait the UI thread here, since some tasks which are performed
  // above message loop back results to the UI thread.
  base::RunLoop().RunUntilIdle();

  // When /a is deleted (1 and 2 is notified).
  FileChange first_change;
  first_change.Update(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/a")),
      FileType::FILE_TYPE_DIRECTORY, ChangeType::CHANGE_TYPE_DELETE);
  event_router_->OnFileChanged(first_change);
  EXPECT_EQ(2, counter);

  // When /a/b/c is deleted (1 is notified).
  FileChange second_change;
  second_change.Update(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/a/b/c")),
      FileType::FILE_TYPE_DIRECTORY, ChangeType::CHANGE_TYPE_DELETE);
  event_router_->OnFileChanged(second_change);
  EXPECT_EQ(3, counter);

  // When /z/y is deleted (Not notified).
  FileChange third_change;
  third_change.Update(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/z/y")),
      FileType::FILE_TYPE_DIRECTORY, ChangeType::CHANGE_TYPE_DELETE);
  event_router_->OnFileChanged(third_change);
  EXPECT_EQ(3, counter);

  // Remove file watchers.
  event_router_->RemoveFileWatch(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/a/b/c")),
      "extension_1");
  event_router_->RemoveFileWatch(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/a/d/e")),
      "extension_2");
  event_router_->RemoveFileWatch(
      base::FilePath(FILE_PATH_LITERAL("/no-existing-fs/root/aaa")),
      "extension_3");

  // event_router->addFileWatch create some tasks which are performed on
  // TaskScheduler. Wait until they are done.
  base::TaskScheduler::GetInstance()->FlushForTesting();
}

IN_PROC_BROWSER_TEST_F(FileManagerPrivateApiTest, ContentChecksum) {
  base::ScopedTempDir temp_dir;
  base::FilePath mount_point_dir;
  const char kLocalMountPointName[] = "local";

  ASSERT_TRUE(InitializeLocalFileSystem(kLocalMountPointName, &temp_dir,
                                        &mount_point_dir))
      << "Failed to initialize test file system";

  EXPECT_TRUE(content::BrowserContext::GetMountPoints(browser()->profile())
                  ->RegisterFileSystem(
                      kLocalMountPointName, storage::kFileSystemTypeNativeLocal,
                      storage::FileSystemMountOption(), mount_point_dir));
  file_manager::VolumeManager::Get(browser()->profile())
      ->AddVolumeForTesting(mount_point_dir, file_manager::VOLUME_TYPE_TESTING,
                            chromeos::DEVICE_TYPE_UNKNOWN,
                            false /* read_only */);

  ASSERT_TRUE(RunComponentExtensionTest("file_browser/content_checksum_test"));
}

IN_PROC_BROWSER_TEST_F(FileManagerPrivateApiTest, Recent) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const base::FilePath downloads_dir = temp_dir.GetPath();

  ASSERT_TRUE(file_manager::VolumeManager::Get(browser()->profile())
                  ->RegisterDownloadsDirectoryForTesting(downloads_dir));

  // Create an empty file.
  {
    base::File file(downloads_dir.Append("all-justice.jpg"),
                    base::File::FLAG_CREATE | base::File::FLAG_WRITE);
    ASSERT_TRUE(file.IsValid());
  }

  ASSERT_TRUE(RunComponentExtensionTest("file_browser/recent_test"));
}
