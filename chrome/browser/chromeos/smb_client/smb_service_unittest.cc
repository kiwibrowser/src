// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_service.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "chrome/browser/chromeos/file_system_provider/fake_provided_file_system.h"
#include "chrome/browser/chromeos/file_system_provider/fake_registry.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/extension_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace smb_client {

class SmbServiceTest : public testing::Test {
 protected:
  SmbServiceTest() : profile_(NULL) {
    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    EXPECT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(
        "test-user@example.com");  // Not owned by profile_.
    std::unique_ptr<FakeChromeUserManager> user_manager_temp =
        std::make_unique<FakeChromeUserManager>();

    user_manager_temp->AddUser(
        AccountId::FromUserEmail(profile_->GetProfileUserName()));

    user_manager_enabler_ = std::make_unique<user_manager::ScopedUserManager>(
        std::move(user_manager_temp));

    // Create FSP service.
    extension_registry_ =
        std::make_unique<extensions::ExtensionRegistry>(profile_);
    fsp_service_ = std::make_unique<file_system_provider::Service>(
        profile_, extension_registry_.get());

    fsp_service_->SetRegistryForTesting(
        std::make_unique<file_system_provider::FakeRegistry>());

    // Create smb service.
    smb_service_ = std::make_unique<SmbService>(profile_);
  }

  ~SmbServiceTest() override {}

  content::TestBrowserThreadBundle
      thread_bundle_;        // Included so tests magically don't crash.
  TestingProfile* profile_;  // Not owned.
  std::unique_ptr<TestingProfileManager> profile_manager_;
  std::unique_ptr<user_manager::ScopedUserManager> user_manager_enabler_;
  std::unique_ptr<SmbService> smb_service_;

  std::unique_ptr<file_system_provider::Service> fsp_service_;
  // Extension Registry and Registry needed for fsp_service.
  std::unique_ptr<extensions::ExtensionRegistry> extension_registry_;
};

}  // namespace smb_client
}  // namespace chromeos
