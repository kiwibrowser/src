// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_manager/path_util.h"

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/sys_info.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_operation_runner.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/scoped_set_running_on_chromeos_for_testing.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/account_id/account_id.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/test/connection_holder_util.h"
#include "components/arc/test/fake_file_system_instance.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "testing/gtest/include/gtest/gtest.h"

using storage::FileSystemURL;

namespace file_manager {
namespace util {
namespace {

const char kLsbRelease[] =
    "CHROMEOS_RELEASE_NAME=Chrome OS\n"
    "CHROMEOS_RELEASE_VERSION=1.2.3.4\n";

TEST(FileManagerPathUtilTest, MultiProfileDownloadsFolderMigration) {
  content::TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;
  // MigratePathFromOldFormat is explicitly disabled on Linux build.
  // So we need to fake that this is real ChromeOS system.
  chromeos::ScopedSetRunningOnChromeOSForTesting fake_release(kLsbRelease,
                                                              base::Time());

  // This looks like "/home/chronos/u-hash/Downloads" in the production
  // environment.
  const base::FilePath kDownloads = GetDownloadsFolderForProfile(&profile);
  const base::FilePath kOldDownloads =
      DownloadPrefs::GetDefaultDownloadDirectory();

  base::FilePath path;

  EXPECT_TRUE(MigratePathFromOldFormat(&profile, kOldDownloads, &path));
  EXPECT_EQ(kDownloads, path);

  EXPECT_TRUE(MigratePathFromOldFormat(
      &profile,
      kOldDownloads.AppendASCII("a/b"),
      &path));
  EXPECT_EQ(kDownloads.AppendASCII("a/b"), path);

  // Path already in the new format is not converted.
  EXPECT_FALSE(MigratePathFromOldFormat(
      &profile,
      kDownloads.AppendASCII("a/b"),
      &path));

  // Only the "Downloads" path is converted.
  EXPECT_FALSE(MigratePathFromOldFormat(
      &profile,
      base::FilePath::FromUTF8Unsafe("/home/chronos/user/dl"),
      &path));
}

std::unique_ptr<KeyedService> CreateFileSystemOperationRunnerForTesting(
    content::BrowserContext* context) {
  return arc::ArcFileSystemOperationRunner::CreateForTesting(
      context, arc::ArcServiceManager::Get()->arc_bridge_service());
}

class FileManagerPathUtilConvertUrlTest : public testing::Test {
 public:
  FileManagerPathUtilConvertUrlTest() = default;
  ~FileManagerPathUtilConvertUrlTest() override = default;

  void SetUp() override {
    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());

    // Set up fake user manager.
    chromeos::FakeChromeUserManager* fake_user_manager =
        new chromeos::FakeChromeUserManager();
    const AccountId account_id(
        AccountId::FromUserEmailGaiaId("user@gmail.com", "1111111111"));
    const AccountId account_id_2(
        AccountId::FromUserEmailGaiaId("user2@gmail.com", "2222222222"));
    fake_user_manager->AddUser(account_id);
    fake_user_manager->LoginUser(account_id);
    fake_user_manager->AddUser(account_id_2);
    fake_user_manager->LoginUser(account_id_2);
    user_manager_enabler_ = std::make_unique<user_manager::ScopedUserManager>(
        base::WrapUnique(std::move(fake_user_manager)));

    Profile* primary_profile =
        profile_manager_->CreateTestingProfile("user@gmail.com");
    ASSERT_TRUE(primary_profile);
    ASSERT_TRUE(profile_manager_->CreateTestingProfile("user2@gmail.com"));

    // Set up an Arc service manager with a fake file system.
    arc_service_manager_ = std::make_unique<arc::ArcServiceManager>();
    arc_service_manager_->set_browser_context(primary_profile);
    arc::ArcFileSystemOperationRunner::GetFactory()->SetTestingFactoryAndUse(
        primary_profile, &CreateFileSystemOperationRunnerForTesting);
    arc_service_manager_->arc_bridge_service()->file_system()->SetInstance(
        &fake_file_system_);
    arc::WaitForInstanceReady(
        arc_service_manager_->arc_bridge_service()->file_system());
    ASSERT_TRUE(fake_file_system_.InitCalled());

    // Add a drive mount point for the primary profile.
    drive_mount_point_ = drive::util::GetDriveMountPointPath(primary_profile);
    const std::string mount_name = drive_mount_point_.BaseName().AsUTF8Unsafe();
    storage::ExternalMountPoints::GetSystemInstance()->RegisterFileSystem(
        mount_name, storage::kFileSystemTypeDrive,
        storage::FileSystemMountOption(), drive_mount_point_);
  }

  void TearDown() override {
    arc_service_manager_->arc_bridge_service()->file_system()->CloseInstance(
        &fake_file_system_);
    user_manager_enabler_.reset();
    profile_manager_.reset();

    // Run all pending tasks before destroying testing profile.
    base::RunLoop().RunUntilIdle();
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  arc::FakeFileSystemInstance fake_file_system_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  std::unique_ptr<user_manager::ScopedUserManager> user_manager_enabler_;
  std::unique_ptr<arc::ArcServiceManager> arc_service_manager_;
  base::FilePath drive_mount_point_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileManagerPathUtilConvertUrlTest);
};

FileSystemURL CreateExternalURL(const base::FilePath& path) {
  return FileSystemURL::CreateForTest(GURL(), storage::kFileSystemTypeExternal,
                                      path);
}

TEST_F(FileManagerPathUtilConvertUrlTest, ConvertPathToArcUrl_Removable) {
  GURL url;
  EXPECT_TRUE(ConvertPathToArcUrl(
      base::FilePath::FromUTF8Unsafe("/media/removable/a/b/c"), &url));
  EXPECT_EQ(GURL("content://org.chromium.arc.removablemediaprovider/a/b/c"),
            url);
}

TEST_F(FileManagerPathUtilConvertUrlTest,
       ConvertPathToArcUrl_InvalidRemovable) {
  GURL url;
  EXPECT_FALSE(ConvertPathToArcUrl(
      base::FilePath::FromUTF8Unsafe("/media/removable_foobar"), &url));
}

TEST_F(FileManagerPathUtilConvertUrlTest, ConvertPathToArcUrl_Downloads) {
  // Conversion of paths under the primary profile's downloads folder.
  GURL url;
  const base::FilePath downloads = GetDownloadsFolderForProfile(
      chromeos::ProfileHelper::Get()->GetProfileByUserIdHashForTest(
          "user@gmail.com-hash"));
  EXPECT_TRUE(ConvertPathToArcUrl(downloads.AppendASCII("a/b/c"), &url));
  EXPECT_EQ(GURL("content://org.chromium.arc.intent_helper.fileprovider/"
                 "download/a/b/c"),
            url);
}

TEST_F(FileManagerPathUtilConvertUrlTest,
       ConvertPathToArcUrl_InvalidDownloads) {
  // Non-primary profile's downloads folder is not supported for ARC yet.
  GURL url;
  const base::FilePath downloads2 = GetDownloadsFolderForProfile(
      chromeos::ProfileHelper::Get()->GetProfileByUserIdHashForTest(
          "user2@gmail.com-hash"));
  EXPECT_FALSE(ConvertPathToArcUrl(downloads2.AppendASCII("a/b/c"), &url));
}

TEST_F(FileManagerPathUtilConvertUrlTest, ConvertPathToArcUrl_Special) {
  GURL url;
  EXPECT_TRUE(
      ConvertPathToArcUrl(drive_mount_point_.AppendASCII("a/b/c"), &url));
  EXPECT_EQ(GURL("content://org.chromium.arc.chromecontentprovider/"
                 "externalfile%3Adrive-user%2540gmail.com-hash%2Fa%2Fb%2Fc"),
            url);
}

TEST_F(FileManagerPathUtilConvertUrlTest,
       ConvertToContentUrls_InvalidMountType) {
  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{FileSystemURL::CreateForTest(
          GURL(), storage::kFileSystemTypeTest,
          base::FilePath::FromUTF8Unsafe("/media/removable/a/b/c"))},
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(1U, urls.size());
            EXPECT_EQ(GURL(), urls[0]);
          },
          &run_loop));
  run_loop.Run();
}

TEST_F(FileManagerPathUtilConvertUrlTest, ConvertToContentUrls_Removable) {
  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{CreateExternalURL(
          base::FilePath::FromUTF8Unsafe("/media/removable/a/b/c"))},
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(1U, urls.size());
            EXPECT_EQ(
                GURL("content://org.chromium.arc.removablemediaprovider/a/b/c"),
                urls[0]);
          },
          &run_loop));
  run_loop.Run();
}

TEST_F(FileManagerPathUtilConvertUrlTest,
       ConvertToContentUrls_InvalidRemovable) {
  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{CreateExternalURL(
          base::FilePath::FromUTF8Unsafe("/media/removable_foobar"))},
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(1U, urls.size());
            EXPECT_EQ(GURL(), urls[0]);
          },
          &run_loop));
  run_loop.Run();
}

TEST_F(FileManagerPathUtilConvertUrlTest, ConvertToContentUrls_Downloads) {
  const base::FilePath downloads = GetDownloadsFolderForProfile(
      chromeos::ProfileHelper::Get()->GetProfileByUserIdHashForTest(
          "user@gmail.com-hash"));
  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{
          CreateExternalURL(downloads.AppendASCII("a/b/c"))},
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(1U, urls.size());
            EXPECT_EQ(
                GURL("content://org.chromium.arc.intent_helper.fileprovider/"
                     "download/a/b/c"),
                urls[0]);
          },
          &run_loop));
  run_loop.Run();
}

TEST_F(FileManagerPathUtilConvertUrlTest,
       ConvertToContentUrls_InvalidDownloads) {
  const base::FilePath downloads = GetDownloadsFolderForProfile(
      chromeos::ProfileHelper::Get()->GetProfileByUserIdHashForTest(
          "user2@gmail.com-hash"));
  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{
          CreateExternalURL(downloads.AppendASCII("a/b/c"))},
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(1U, urls.size());
            EXPECT_EQ(GURL(), urls[0]);
          },
          &run_loop));
  run_loop.Run();
}

TEST_F(FileManagerPathUtilConvertUrlTest, ConvertToContentUrls_Special) {
  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{
          CreateExternalURL(drive_mount_point_.AppendASCII("a/b/c"))},
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(1U, urls.size());
            EXPECT_EQ(
                GURL(
                    "content://org.chromium.arc.chromecontentprovider/"
                    "externalfile%3Adrive-user%2540gmail.com-hash%2Fa%2Fb%2Fc"),
                urls[0]);
          },
          &run_loop));
  run_loop.Run();
}

TEST_F(FileManagerPathUtilConvertUrlTest,
       ConvertToContentUrls_ArcDocumentsProvider) {
  // Add images_root/Download/photo.jpg to the fake file system.
  const char kAuthority[] = "com.android.providers.media.documents";
  fake_file_system_.AddDocument(arc::FakeFileSystemInstance::Document(
      kAuthority, "images_root", "", "", arc::kAndroidDirectoryMimeType, -1,
      0));
  fake_file_system_.AddDocument(arc::FakeFileSystemInstance::Document(
      kAuthority, "dir-id", "images_root", "Download",
      arc::kAndroidDirectoryMimeType, -1, 22));
  fake_file_system_.AddDocument(arc::FakeFileSystemInstance::Document(
      kAuthority, "photo-id", "dir-id", "photo.jpg", "image/jpeg", 3, 33));

  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{FileSystemURL::CreateForTest(
          GURL(), storage::kFileSystemTypeArcDocumentsProvider,
          base::FilePath::FromUTF8Unsafe(
              "/special/arc-documents-provider/"
              "com.android.providers.media.documents/"
              "images_root/Download/photo.jpg"))},
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(1U, urls.size());
            EXPECT_EQ(GURL("content://com.android.providers.media.documents/"
                           "document/photo-id"),
                      urls[0]);
          },
          &run_loop));
  run_loop.Run();
}

TEST_F(FileManagerPathUtilConvertUrlTest,
       ConvertToContentUrls_ArcDocumentsProviderFileNotFound) {
  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{FileSystemURL::CreateForTest(
          GURL(), storage::kFileSystemTypeArcDocumentsProvider,
          base::FilePath::FromUTF8Unsafe(
              "/special/arc-documents-provider/"
              "com.android.providers.media.documents/"
              "images_root/Download/photo.jpg"))},
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(1U, urls.size());
            EXPECT_EQ(GURL(""), urls[0]);
          },
          &run_loop));
  run_loop.Run();
}

TEST_F(FileManagerPathUtilConvertUrlTest, ConvertToContentUrls_MultipeUrls) {
  base::RunLoop run_loop;
  ConvertToContentUrls(
      std::vector<FileSystemURL>{
          CreateExternalURL(base::FilePath::FromUTF8Unsafe("/invalid")),
          CreateExternalURL(
              base::FilePath::FromUTF8Unsafe("/media/removable/a/b/c")),
          CreateExternalURL(drive_mount_point_.AppendASCII("a/b/c")),
      },
      base::BindOnce(
          [](base::RunLoop* run_loop, const std::vector<GURL>& urls) {
            run_loop->Quit();
            ASSERT_EQ(3U, urls.size());
            EXPECT_EQ(GURL(), urls[0]);  // Invalid URL.
            EXPECT_EQ(
                GURL("content://org.chromium.arc.removablemediaprovider/a/b/c"),
                urls[1]);
            EXPECT_EQ(
                GURL(
                    "content://org.chromium.arc.chromecontentprovider/"
                    "externalfile%3Adrive-user%2540gmail.com-hash%2Fa%2Fb%2Fc"),
                urls[2]);
          },
          &run_loop));
  run_loop.Run();
}

}  // namespace
}  // namespace util
}  // namespace file_manager
