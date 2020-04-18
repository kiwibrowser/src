// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/login_ui_service.h"

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

class LoginUIServiceTest : public testing::Test {
 public:
  LoginUIServiceTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()),
        profile_(nullptr) {}
  ~LoginUIServiceTest() override {}

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile("Person 1");
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;

  TestingProfileManager profile_manager_;
  // Test profile used by all tests - this is owned by profile_manager_.
  TestingProfile* profile_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginUIServiceTest);
};

class TestLoginUI : public LoginUIService::LoginUI {
 public:
  TestLoginUI() { }
  ~TestLoginUI() override {}
  void FocusUI() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestLoginUI);
};

TEST_F(LoginUIServiceTest, CanSetMultipleLoginUIs) {
  LoginUIService service(profile_);

  EXPECT_EQ(nullptr, service.current_login_ui());

  TestLoginUI ui;
  service.SetLoginUI(&ui);
  EXPECT_EQ(&ui, service.current_login_ui());

  // Test that we can replace the active login UI.
  TestLoginUI other_ui;
  service.SetLoginUI(&other_ui);
  EXPECT_EQ(&other_ui, service.current_login_ui());

  // Test that closing the non-active login UI has no effect.
  service.LoginUIClosed(&ui);
  EXPECT_EQ(&other_ui, service.current_login_ui());

  // Test that closing the foreground UI yields the background UI.
  service.SetLoginUI(&ui);
  EXPECT_EQ(&ui, service.current_login_ui());
  service.LoginUIClosed(&ui);
  EXPECT_EQ(&other_ui, service.current_login_ui());

  // Test that closing the last login UI makes the current login UI nullptr.
  service.LoginUIClosed(&other_ui);
  EXPECT_EQ(nullptr, service.current_login_ui());
}

TEST_F(LoginUIServiceTest, SetProfileBlockingErrorMessage) {
  LoginUIService service(profile_);

  service.SetProfileBlockingErrorMessage();

  EXPECT_EQ(base::string16(), service.GetLastLoginResult());
  EXPECT_EQ(base::string16(), service.GetLastLoginErrorEmail());
  EXPECT_TRUE(service.IsDisplayingProfileBlockedErrorMessage());
}
