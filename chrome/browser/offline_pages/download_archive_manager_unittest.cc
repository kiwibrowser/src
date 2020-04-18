// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/download_archive_manager.h"

#include "base/macros.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
const char* kPrivateDir = "/private/";
const char* kTemporaryDir = "/temporary/";
const char* kPublicDir = "/public/";
const char* kChromePublicSdCardDir =
    "/sd-card/1234-5678/Android/data/org.chromium.chrome/files/Download";
}  // namespace

class DownloadArchiveManagerTest : public testing::Test {
 public:
  DownloadArchiveManagerTest() = default;
  ~DownloadArchiveManagerTest() override = default;

  void SetUp() override;
  void TearDown() override;
  void PumpLoop();

  TestingProfile* profile() { return &profile_; }
  DownloadArchiveManager* archive_manager() { return archive_manager_.get(); }

 private:
  content::TestBrowserThreadBundle browser_thread_bundle_;
  TestingProfile profile_;
  std::unique_ptr<DownloadArchiveManager> archive_manager_;
  DISALLOW_COPY_AND_ASSIGN(DownloadArchiveManagerTest);
};

void DownloadArchiveManagerTest::SetUp() {
  // Set up preferences to point to kChromePublicSdCardDir.
  profile()->GetPrefs()->SetString(prefs::kDownloadDefaultDirectory,
                                   kChromePublicSdCardDir);

  // Create a DownloadArchiveManager to use.
  archive_manager_.reset(new DownloadArchiveManager(
      base::FilePath(kTemporaryDir), base::FilePath(kPrivateDir),
      base::FilePath(kPublicDir), base::ThreadTaskRunnerHandle::Get(),
      profile()));
}

void DownloadArchiveManagerTest::TearDown() {
  archive_manager_.release();
}

TEST_F(DownloadArchiveManagerTest, UseDownloadDirFromPreferences) {
  base::FilePath download_dir = archive_manager()->GetPublicArchivesDir();
  ASSERT_EQ(kChromePublicSdCardDir, download_dir.AsUTF8Unsafe());
}

TEST_F(DownloadArchiveManagerTest, NullProfile) {
  DownloadArchiveManager download_archive_manager(
      base::FilePath(kTemporaryDir), base::FilePath(kPrivateDir),
      base::FilePath(kPublicDir), base::ThreadTaskRunnerHandle::Get(), nullptr);

  base::FilePath download_dir = download_archive_manager.GetPublicArchivesDir();
  ASSERT_EQ(kPublicDir, download_dir.AsUTF8Unsafe());
}

}  // namespace offline_pages
