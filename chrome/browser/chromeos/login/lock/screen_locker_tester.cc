// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/lock/screen_locker_tester.h"

#include <string>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/browser/chromeos/login/lock/webui_screen_locker.h"
#include "chromeos/login/auth/auth_status_consumer.h"
#include "chromeos/login/auth/fake_extended_authenticator.h"
#include "chromeos/login/auth/stub_authenticator.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/widget/root_view.h"

using content::WebContents;

namespace {

// This class is used to observe state of the global ScreenLocker instance,
// which can go away as a result of a successful authentication. As such,
// it needs to directly reference the global ScreenLocker.
class LoginAttemptObserver : public chromeos::AuthStatusConsumer {
 public:
  LoginAttemptObserver();
  ~LoginAttemptObserver() override;

  void WaitForAttempt();

  // Overridden from AuthStatusConsumer:
  void OnAuthFailure(const chromeos::AuthFailure& error) override {
    LoginAttempted();
  }

  void OnAuthSuccess(const chromeos::UserContext& credentials) override {
    LoginAttempted();
  }

 private:
  void LoginAttempted();

  bool login_attempted_;
  bool waiting_;

  DISALLOW_COPY_AND_ASSIGN(LoginAttemptObserver);
};

LoginAttemptObserver::LoginAttemptObserver()
    : chromeos::AuthStatusConsumer(), login_attempted_(false), waiting_(false) {
  chromeos::ScreenLocker::default_screen_locker()->SetLoginStatusConsumer(this);
}

LoginAttemptObserver::~LoginAttemptObserver() {
  chromeos::ScreenLocker* global_locker =
      chromeos::ScreenLocker::default_screen_locker();
  if (global_locker)
    global_locker->SetLoginStatusConsumer(NULL);
}

void LoginAttemptObserver::WaitForAttempt() {
  if (!login_attempted_) {
    waiting_ = true;
    content::RunMessageLoop();
    waiting_ = false;
  }
  ASSERT_TRUE(login_attempted_);
}

void LoginAttemptObserver::LoginAttempted() {
  login_attempted_ = true;
  if (waiting_)
    base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

}  // namespace

namespace chromeos {

namespace test {

class WebUIScreenLockerTester : public ScreenLockerTester {
 public:
  // ScreenLockerTester overrides:
  void SetPassword(const std::string& password) override;
  std::string GetPassword() override;
  void EnterPassword(const std::string& password) override;
  void EmulateWindowManagerReady() override;
  views::Widget* GetWidget() const override;
  views::Widget* GetChildWidget() const override;

 private:
  friend class chromeos::ScreenLocker;

  WebUIScreenLockerTester() {}

  content::RenderViewHost* RenderViewHost() const;

  // Returns the ScreenLockerWebUI object.
  WebUIScreenLocker* webui_screen_locker() const;

  // Returns the WebUI object from the screen locker.
  content::WebUI* webui() const;

  DISALLOW_COPY_AND_ASSIGN(WebUIScreenLockerTester);
};

void WebUIScreenLockerTester::SetPassword(const std::string& password) {
  webui()->GetWebContents()->GetMainFrame()->ExecuteJavaScriptForTests(
      base::ASCIIToUTF16(base::StringPrintf(
          "$('pod-row').pods[0].passwordElement.value = '%s';",
          password.c_str())));
}

std::string WebUIScreenLockerTester::GetPassword() {
  std::string result;
  std::unique_ptr<base::Value> v = content::ExecuteScriptAndGetValue(
      RenderViewHost()->GetMainFrame(),
      "$('pod-row').pods[0].passwordElement.value;");
  CHECK(v->GetAsString(&result));
  return result;
}

void WebUIScreenLockerTester::EnterPassword(const std::string& password) {
  bool result;
  SetPassword(password);

  // Verify password is set.
  ASSERT_EQ(password, GetPassword());

  // Verify that "reauth" warning is hidden.
  std::unique_ptr<base::Value> v = content::ExecuteScriptAndGetValue(
      RenderViewHost()->GetMainFrame(),
      "window.getComputedStyle("
      "    $('pod-row').pods[0].querySelector('.reauth-hint-container'))"
      "        .display == 'none'");
  ASSERT_TRUE(v->GetAsBoolean(&result));
  ASSERT_TRUE(result);

  // Attempt to sign in.
  LoginAttemptObserver login;
  v = content::ExecuteScriptAndGetValue(RenderViewHost()->GetMainFrame(),
                                        "$('pod-row').pods[0].activate();");
  ASSERT_TRUE(v->GetAsBoolean(&result));
  ASSERT_TRUE(result);

  // Wait for login attempt.
  login.WaitForAttempt();
}

void WebUIScreenLockerTester::EmulateWindowManagerReady() {}

views::Widget* WebUIScreenLockerTester::GetWidget() const {
  return webui_screen_locker()->lock_window_;
}

views::Widget* WebUIScreenLockerTester::GetChildWidget() const {
  return webui_screen_locker()->lock_window_;
}

content::RenderViewHost* WebUIScreenLockerTester::RenderViewHost() const {
  return webui()->GetWebContents()->GetRenderViewHost();
}

WebUIScreenLocker* WebUIScreenLockerTester::webui_screen_locker() const {
  DCHECK(ScreenLocker::screen_locker_);
  return ScreenLocker::screen_locker_->web_ui_for_testing();
}

content::WebUI* WebUIScreenLockerTester::webui() const {
  DCHECK(webui_screen_locker()->webui_ready_);
  content::WebUI* webui = webui_screen_locker()->GetWebUI();
  DCHECK(webui);
  return webui;
}

ScreenLockerTester::ScreenLockerTester() {}

ScreenLockerTester::~ScreenLockerTester() {}

bool ScreenLockerTester::IsLocked() {
  return ScreenLocker::screen_locker_ && ScreenLocker::screen_locker_->locked_;
}

void ScreenLockerTester::InjectStubUserContext(
    const UserContext& user_context) {
  DCHECK(ScreenLocker::screen_locker_);
  ScreenLocker::screen_locker_->SetAuthenticator(
      new StubAuthenticator(ScreenLocker::screen_locker_, user_context));
  ScreenLocker::screen_locker_->extended_authenticator_ =
      new FakeExtendedAuthenticator(ScreenLocker::screen_locker_, user_context);
}

}  // namespace test

test::ScreenLockerTester* ScreenLocker::GetTester() {
  return new test::WebUIScreenLockerTester();
}

}  // namespace chromeos
