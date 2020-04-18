// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "ash/public/cpp/config.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/test/oobe_base_test.h"
#include "chrome/browser/chromeos/login/ui/login_display_webui.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/chromeos_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "google_apis/gaia/fake_gaia.h"
#include "google_apis/gaia/gaia_switches.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_response.h"
#include "rlz/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_RLZ)
#include "base/task_scheduler/post_task.h"
#include "components/rlz/rlz_tracker.h"
#endif

namespace chromeos {

namespace {

#if BUILDFLAG(ENABLE_RLZ)
void GetAccessPointRlzInBackgroundThread(rlz_lib::AccessPoint point,
                                         base::string16* rlz) {
  ASSERT_FALSE(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  ASSERT_TRUE(rlz::RLZTracker::GetAccessPointRlz(point, rlz));
}
#endif

}  // namespace

class LoginUtilsTest : public OobeBaseTest {
 public:
  LoginUtilsTest() {}

  void RunUntilIdle() { base::RunLoop().RunUntilIdle(); }

  PrefService* local_state() { return g_browser_process->local_state(); }

  void Login(const std::string& username) {
    content::WindowedNotificationObserver session_started_observer(
        chrome::NOTIFICATION_SESSION_STARTED,
        content::NotificationService::AllSources());

    ExistingUserController* controller =
        ExistingUserController::current_controller();
    ASSERT_TRUE(controller);
    LoginDisplayWebUI* login_display =
        static_cast<LoginDisplayWebUI*>(controller->login_display());
    ASSERT_TRUE(login_display);

    login_display->ShowSigninScreenForTest(username, "password", "[]");

    // Wait for the session to start after submitting the credentials. This
    // will wait until all the background requests are done.
    session_started_observer.Wait();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginUtilsTest);
};

// Exercises login, like the desktopui_MashLogin Chrome OS autotest.
IN_PROC_BROWSER_TEST_F(LoginUtilsTest, MashLogin) {
  if (GetAshConfig() != ash::Config::MASH)
    return;

  WaitForSigninScreen();
  Login("username");
  // Login did not time out and did not crash.
}

#if BUILDFLAG(ENABLE_RLZ)
IN_PROC_BROWSER_TEST_F(LoginUtilsTest, RlzInitialized) {
  WaitForSigninScreen();

  // No RLZ brand code set initially.
  EXPECT_FALSE(local_state()->HasPrefPath(prefs::kRLZBrand));

  // Wait for blocking RLZ tasks to complete.
  {
    base::RunLoop loop;
    PrefChangeRegistrar registrar;
    registrar.Init(local_state());
    registrar.Add(prefs::kRLZBrand, loop.QuitClosure());
    Login("username");
    loop.Run();
  }

  // RLZ brand code has been set to empty string.
  EXPECT_TRUE(local_state()->HasPrefPath(prefs::kRLZBrand));
  EXPECT_EQ(std::string(), local_state()->GetString(prefs::kRLZBrand));

  // RLZ value for homepage access point should have been initialized.
  // This value must be obtained in a background thread.
  {
    base::RunLoop loop;
    base::string16 rlz_string;
    base::PostTaskWithTraitsAndReply(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::Bind(&GetAccessPointRlzInBackgroundThread,
                   rlz::RLZTracker::ChromeHomePage(), &rlz_string),
        loop.QuitClosure());
    loop.Run();
    EXPECT_EQ(base::string16(), rlz_string);
  }
}
#endif

}  // namespace chromeos
