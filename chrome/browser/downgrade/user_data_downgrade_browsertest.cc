// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/downgrade/user_data_downgrade.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/test/test_reg_util_win.h"
#include "base/threading/thread_restrictions.h"
#include "base/version.h"
#include "base/win/registry.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browser_thread.h"

namespace downgrade {

class UserDataDowngradeBrowserTestBase : public InProcessBrowserTest {
 protected:
  // content::BrowserTestBase:
  void SetUpInProcessBrowserTestFixture() override {
    HKEY root = HKEY_CURRENT_USER;
    ASSERT_NO_FATAL_FAILURE(registry_override_manager_.OverrideRegistry(root));
    key_.Create(root,
                BrowserDistribution::GetDistribution()->GetStateKey().c_str(),
                KEY_SET_VALUE | KEY_WOW64_32KEY);
  }

  // InProcessBrowserTest:
  bool SetUpUserDataDirectory() override {
    if (!base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir_))
      return false;
    if (!CreateTemporaryFileInDir(user_data_dir_, &other_file_))
      return false;
    last_version_file_path_ = user_data_dir_.Append(kDowngradeLastVersionFile);
    std::string last_version = GetNextChromeVersion();
    base::WriteFile(last_version_file_path_, last_version.c_str(),
                    last_version.size());

    moved_user_data_dir_ =
        base::FilePath(user_data_dir_.value() + FILE_PATH_LITERAL(" (1)"))
            .AddExtension(kDowngradeDeleteSuffix);

    return true;
  }

  // content::BrowserTestBase:
  // Verify the renamed user data directory has been deleted.
  void TearDownInProcessBrowserTestFixture() override {
    ASSERT_FALSE(base::DirectoryExists(moved_user_data_dir_));
    key_.Close();
  }

  std::string GetNextChromeVersion() {
    return base::Version(std::string(chrome::kChromeVersion) + "1").GetString();
  }

  base::FilePath last_version_file_path_;
  // The path to an arbitrary file in the user data dir that will be present
  // only when a reset does not take place.
  base::FilePath other_file_;
  base::FilePath user_data_dir_;
  base::FilePath moved_user_data_dir_;
  base::win::RegKey key_;
  registry_util::RegistryOverrideManager registry_override_manager_;
};

class UserDataDowngradeBrowserCopyAndCleanTest
    : public UserDataDowngradeBrowserTestBase {
 protected:
  // content::BrowserTestBase:
  void SetUpInProcessBrowserTestFixture() override {
    UserDataDowngradeBrowserTestBase::SetUpInProcessBrowserTestFixture();
    key_.WriteValue(L"DowngradeVersion",
                    base::ASCIIToUTF16(GetNextChromeVersion()).c_str());
    key_.WriteValue(google_update::kRegMSIField, 1);
  }

  // InProcessBrowserTest:
  // Verify the content of renamed user data directory.
  void SetUpOnMainThread() override {
    ASSERT_TRUE(base::DirectoryExists(moved_user_data_dir_));
    ASSERT_TRUE(
        base::PathExists(moved_user_data_dir_.Append(other_file_.BaseName())));
    EXPECT_EQ(GetNextChromeVersion(),
              GetLastVersion(moved_user_data_dir_).GetString());
    UserDataDowngradeBrowserTestBase::SetUpOnMainThread();
  }
};

class UserDataDowngradeBrowserNoResetTest
    : public UserDataDowngradeBrowserTestBase {
 protected:
  // content::BrowserTestBase:
  void SetUpInProcessBrowserTestFixture() override {
    UserDataDowngradeBrowserTestBase::SetUpInProcessBrowserTestFixture();
    key_.WriteValue(google_update::kRegMSIField, 1);
  }
};

class UserDataDowngradeBrowserNoMSITest
    : public UserDataDowngradeBrowserTestBase {
 protected:
  // InProcessBrowserTest:
  bool SetUpUserDataDirectory() override {
    return CreateTemporaryFileInDir(user_data_dir_, &other_file_);
  }
};

// Verify the user data directory has been renamed and created again after
// downgrade.
IN_PROC_BROWSER_TEST_F(UserDataDowngradeBrowserCopyAndCleanTest, Test) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::TaskScheduler::GetInstance()->FlushForTesting();
  EXPECT_EQ(chrome::kChromeVersion, GetLastVersion(user_data_dir_).GetString());
  ASSERT_FALSE(base::PathExists(other_file_));
}

// Verify the user data directory will not be reset without downgrade.
IN_PROC_BROWSER_TEST_F(UserDataDowngradeBrowserNoResetTest, Test) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_EQ(chrome::kChromeVersion, GetLastVersion(user_data_dir_).GetString());
  ASSERT_TRUE(base::PathExists(other_file_));
}

// Verify the "Last Version" file won't be created for non-msi install.
IN_PROC_BROWSER_TEST_F(UserDataDowngradeBrowserNoMSITest, Test) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  ASSERT_FALSE(base::PathExists(last_version_file_path_));
  ASSERT_TRUE(base::PathExists(other_file_));
}

}  // namespace downgrade
