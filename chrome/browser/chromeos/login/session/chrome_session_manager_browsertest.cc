// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/session/chrome_session_manager.h"

#include <memory>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/login_manager_test.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/ui/login_display_webui.h"
#include "chrome/browser/chromeos/login/ui/user_adding_screen.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/test/test_utils.h"
#include "google_apis/gaia/fake_gaia.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace {

struct {
  const char* email;
  const char* gaia_id;
} const kTestUsers[] = {{"test-user1@consumer.example.com", "1111111111"},
                        {"test-user2@consumer.example.com", "2222222222"},
                        {"test-user3@consumer.example.com", "3333333333"}};

// Helper class to wait for user adding screen to finish.
class UserAddingScreenWaiter : public UserAddingScreen::Observer {
 public:
  UserAddingScreenWaiter() { UserAddingScreen::Get()->AddObserver(this); }
  ~UserAddingScreenWaiter() override {
    UserAddingScreen::Get()->RemoveObserver(this);
  }

  void Wait() {
    if (!UserAddingScreen::Get()->IsRunning())
      return;
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

  // UserAddingScreen::Observer:
  void OnUserAddingFinished() override {
    if (run_loop_)
      run_loop_->Quit();
  }

 private:
  std::unique_ptr<base::RunLoop> run_loop_;

  DISALLOW_COPY_AND_ASSIGN(UserAddingScreenWaiter);
};

}  // anonymous namespace

class ChromeSessionManagerTest : public LoginManagerTest {
 public:
  ChromeSessionManagerTest() : LoginManagerTest(true) {}
  ~ChromeSessionManagerTest() override {}

  // LoginManagerTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    LoginManagerTest::SetUpCommandLine(command_line);

    command_line->AppendSwitch(switches::kOobeSkipPostLogin);
  }

  void StartSignInScreen() {
    WizardController* wizard_controller =
        WizardController::default_controller();
    ASSERT_TRUE(wizard_controller);
    wizard_controller->SkipToLoginForTesting(LoginScreenContext());
    OobeScreenWaiter(OobeScreen::SCREEN_GAIA_SIGNIN).Wait();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeSessionManagerTest);
};

IN_PROC_BROWSER_TEST_F(ChromeSessionManagerTest, OobeNewUser) {
  // Verify that session state is OOBE and no user sessions in fresh start.
  session_manager::SessionManager* manager =
      session_manager::SessionManager::Get();
  EXPECT_EQ(session_manager::SessionState::OOBE, manager->session_state());
  EXPECT_EQ(0u, manager->sessions().size());

  // Login via fake gaia to add a new user.
  fake_gaia_.SetFakeMergeSessionParams(kTestUsers[0].email, "fake_sid",
                                       "fake_lsid");
  StartSignInScreen();

  content::WindowedNotificationObserver session_start_waiter(
      chrome::NOTIFICATION_SESSION_STARTED,
      content::NotificationService::AllSources());

  LoginDisplayWebUI* login_display = static_cast<LoginDisplayWebUI*>(
      ExistingUserController::current_controller()->login_display());
  login_display->ShowSigninScreenForTest(kTestUsers[0].email, "fake_password",
                                         "[]");

  session_start_waiter.Wait();

  // Verify that session state is ACTIVE with one user session.
  EXPECT_EQ(session_manager::SessionState::ACTIVE, manager->session_state());
  EXPECT_EQ(1u, manager->sessions().size());
}

IN_PROC_BROWSER_TEST_F(ChromeSessionManagerTest, PRE_LoginExistingUsers) {
  for (const auto& user : kTestUsers) {
    RegisterUser(AccountId::FromUserEmailGaiaId(user.email, user.gaia_id));
  }
  StartupUtils::MarkOobeCompleted();
}

IN_PROC_BROWSER_TEST_F(ChromeSessionManagerTest, LoginExistingUsers) {
  // Verify that session state is LOGIN_PRIMARY with existing user data dir.
  session_manager::SessionManager* manager =
      session_manager::SessionManager::Get();
  EXPECT_EQ(session_manager::SessionState::LOGIN_PRIMARY,
            manager->session_state());
  EXPECT_EQ(0u, manager->sessions().size());

  std::vector<AccountId> test_users;
  test_users.push_back(AccountId::FromUserEmailGaiaId(kTestUsers[0].email,
                                                      kTestUsers[0].gaia_id));
  // Verify that session state is ACTIVE with one user session after signing
  // in a user.
  LoginUser(test_users[0]);
  EXPECT_EQ(session_manager::SessionState::ACTIVE, manager->session_state());
  EXPECT_EQ(1u, manager->sessions().size());

  for (size_t i = 1; i < arraysize(kTestUsers); ++i) {
    // Verify that session state is LOGIN_SECONDARY during user adding.
    UserAddingScreen::Get()->Start();
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(session_manager::SessionState::LOGIN_SECONDARY,
              manager->session_state());

    // Verify that session state is ACTIVE with 1+i user sessions after user
    // is added and new user session is started..
    UserAddingScreenWaiter waiter;
    test_users.push_back(AccountId::FromUserEmailGaiaId(kTestUsers[i].email,
                                                        kTestUsers[i].gaia_id));
    AddUser(test_users.back());
    waiter.Wait();
    base::RunLoop().RunUntilIdle();

    EXPECT_EQ(session_manager::SessionState::ACTIVE, manager->session_state());
    EXPECT_EQ(1u + i, manager->sessions().size());
  }

  // Verify that session manager has the correct user session info.
  ASSERT_EQ(test_users.size(), manager->sessions().size());
  for (size_t i = 0; i < test_users.size(); ++i) {
    EXPECT_EQ(test_users[i], manager->sessions()[i].user_account_id);
  }
}

}  // namespace chromeos
