// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/file_system_util.h"

#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/drive/test_util.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "storage/browser/fileapi/file_system_backend.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/browser/test/test_file_system_options.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace util {

namespace {

// Sets up ProfileManager for testing and marks the current thread as UI by
// TestBrowserThreadBundle. We need the thread since Profile objects must be
// touched from UI and hence has CHECK/DCHECKs for it.
class ProfileRelatedFileSystemUtilTest : public testing::Test {
 protected:
  ProfileRelatedFileSystemUtilTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {}

  void SetUp() override { ASSERT_TRUE(testing_profile_manager_.SetUp()); }

  TestingProfileManager& testing_profile_manager() {
    return testing_profile_manager_;
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfileManager testing_profile_manager_;
};

}  // namespace

TEST_F(ProfileRelatedFileSystemUtilTest, GetDriveMountPointPath) {
  Profile* profile = testing_profile_manager().CreateTestingProfile("user1");
  const std::string user_id_hash =
      chromeos::ProfileHelper::GetUserIdHashByUserIdForTesting("user1");
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("/special/drive-" + user_id_hash),
            GetDriveMountPointPath(profile));
}

TEST_F(ProfileRelatedFileSystemUtilTest, IsUnderDriveMountPoint) {
  EXPECT_FALSE(IsUnderDriveMountPoint(
      base::FilePath::FromUTF8Unsafe("/wherever/foo.txt")));
  EXPECT_FALSE(IsUnderDriveMountPoint(
      base::FilePath::FromUTF8Unsafe("/special/foo.txt")));
  EXPECT_FALSE(IsUnderDriveMountPoint(
      base::FilePath::FromUTF8Unsafe("special/drive/foo.txt")));

  EXPECT_TRUE(
      IsUnderDriveMountPoint(base::FilePath::FromUTF8Unsafe("/special/drive")));
  EXPECT_TRUE(IsUnderDriveMountPoint(
      base::FilePath::FromUTF8Unsafe("/special/drive/foo.txt")));
  EXPECT_TRUE(IsUnderDriveMountPoint(
      base::FilePath::FromUTF8Unsafe("/special/drive/subdir/foo.txt")));
  EXPECT_TRUE(IsUnderDriveMountPoint(
      base::FilePath::FromUTF8Unsafe("/special/drive-xxx/foo.txt")));
}

TEST_F(ProfileRelatedFileSystemUtilTest, ExtractDrivePath) {
  EXPECT_EQ(
      base::FilePath(),
      ExtractDrivePath(base::FilePath::FromUTF8Unsafe("/wherever/foo.txt")));
  EXPECT_EQ(
      base::FilePath(),
      ExtractDrivePath(base::FilePath::FromUTF8Unsafe("/special/foo.txt")));

  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("drive"),
            ExtractDrivePath(base::FilePath::FromUTF8Unsafe("/special/drive")));
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("drive/foo.txt"),
            ExtractDrivePath(
                base::FilePath::FromUTF8Unsafe("/special/drive/foo.txt")));
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("drive/subdir/foo.txt"),
            ExtractDrivePath(base::FilePath::FromUTF8Unsafe(
                "/special/drive/subdir/foo.txt")));
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("drive/foo.txt"),
            ExtractDrivePath(
                base::FilePath::FromUTF8Unsafe("/special/drive-xxx/foo.txt")));
}

TEST_F(ProfileRelatedFileSystemUtilTest, ExtractProfileFromPath) {
  Profile* profile1 = testing_profile_manager().CreateTestingProfile("user1");
  Profile* profile2 = testing_profile_manager().CreateTestingProfile("user2");
  const std::string user1_id_hash =
      chromeos::ProfileHelper::GetUserIdHashByUserIdForTesting("user1");
  const std::string user2_id_hash =
      chromeos::ProfileHelper::GetUserIdHashByUserIdForTesting("user2");
  EXPECT_EQ(profile1, ExtractProfileFromPath(base::FilePath::FromUTF8Unsafe(
                          "/special/drive-" + user1_id_hash)));
  EXPECT_EQ(profile2, ExtractProfileFromPath(base::FilePath::FromUTF8Unsafe(
                          "/special/drive-" + user2_id_hash + "/root/xxx")));
  EXPECT_EQ(NULL, ExtractProfileFromPath(base::FilePath::FromUTF8Unsafe(
                      "/special/non-drive-path")));
}

TEST_F(ProfileRelatedFileSystemUtilTest, ExtractDrivePathFromFileSystemUrl) {
  TestingProfile profile;

  // Set up file system context for testing.
  base::ScopedTempDir temp_dir_;
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

  scoped_refptr<storage::ExternalMountPoints> mount_points =
      storage::ExternalMountPoints::CreateRefCounted();
  scoped_refptr<storage::FileSystemContext> context(
      new storage::FileSystemContext(
          base::ThreadTaskRunnerHandle::Get().get(),
          base::ThreadTaskRunnerHandle::Get().get(), mount_points.get(),
          NULL,  // special_storage_policy
          NULL,  // quota_manager_proxy,
          std::vector<std::unique_ptr<storage::FileSystemBackend>>(),
          std::vector<storage::URLRequestAutoMountHandler>(),
          temp_dir_.GetPath(),  // partition_path
          content::CreateAllowFileAccessOptions()));

  // Type:"external" + virtual_path:"drive/foo/bar" resolves to "drive/foo/bar".
  const std::string& drive_mount_name =
      GetDriveMountPointPath(&profile).BaseName().AsUTF8Unsafe();
  mount_points->RegisterFileSystem(
      drive_mount_name, storage::kFileSystemTypeDrive,
      storage::FileSystemMountOption(), GetDriveMountPointPath(&profile));
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("drive/foo/bar"),
            ExtractDrivePathFromFileSystemUrl(context->CrackURL(
                GURL("filesystem:chrome-extension://dummy-id/external/" +
                     drive_mount_name + "/foo/bar"))));

  // Virtual mount name should not affect the extracted path.
  mount_points->RevokeFileSystem(drive_mount_name);
  mount_points->RegisterFileSystem("drive2", storage::kFileSystemTypeDrive,
                                   storage::FileSystemMountOption(),
                                   GetDriveMountPointPath(&profile));
  EXPECT_EQ(
      base::FilePath::FromUTF8Unsafe("drive/foo/bar"),
      ExtractDrivePathFromFileSystemUrl(context->CrackURL(GURL(
          "filesystem:chrome-extension://dummy-id/external/drive2/foo/bar"))));

  // Type:"external" + virtual_path:"Downloads/foo" is not a Drive path.
  mount_points->RegisterFileSystem(
      "Downloads", storage::kFileSystemTypeNativeLocal,
      storage::FileSystemMountOption(), temp_dir_.GetPath());
  EXPECT_EQ(
      base::FilePath(),
      ExtractDrivePathFromFileSystemUrl(context->CrackURL(GURL(
          "filesystem:chrome-extension://dummy-id/external/Downloads/foo"))));

  // Type:"isolated" + virtual_path:"isolated_id/name" mapped on a Drive path.
  std::string isolated_name;
  std::string isolated_id =
      storage::IsolatedContext::GetInstance()->RegisterFileSystemForPath(
          storage::kFileSystemTypeNativeForPlatformApp, std::string(),
          GetDriveMountPointPath(&profile).AppendASCII("bar/buz"),
          &isolated_name);
  EXPECT_EQ(base::FilePath::FromUTF8Unsafe("drive/bar/buz"),
            ExtractDrivePathFromFileSystemUrl(context->CrackURL(
                GURL("filesystem:chrome-extension://dummy-id/isolated/" +
                     isolated_id + "/" + isolated_name))));
}

TEST_F(ProfileRelatedFileSystemUtilTest, GetCacheRootPath) {
  TestingProfile profile;
  base::FilePath profile_path = profile.GetPath();
  EXPECT_EQ(profile_path.AppendASCII("GCache/v1"),
            util::GetCacheRootPath(&profile));
}

}  // namespace util
}  // namespace drive
