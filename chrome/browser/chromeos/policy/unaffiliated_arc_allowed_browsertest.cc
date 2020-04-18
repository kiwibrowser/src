// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/policy/affiliation_test_helper.h"
#include "chrome/browser/chromeos/policy/device_policy_builder.h"
#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/settings/cros_settings_names.h"
#include "components/arc/arc_util.h"
#include "components/policy/proto/chrome_device_policy.pb.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

namespace policy {

namespace {

constexpr char kAffiliatedUserEmail[] = "affiliated-user@example.com";
constexpr char kAffiliatedUserGaiaId[] = "affiliated-user@example.com";
constexpr char kAffiliationID[] = "some-affiliation-id";
constexpr char kAnotherAffiliationID[] = "another-affiliation-id";

struct Params {
  explicit Params(bool affiliated) : affiliated_(affiliated) {}
  bool affiliated_;
};

}  // namespace

class UnaffiliatedArcAllowedTest
    : public DevicePolicyCrosBrowserTest,
      public ::testing::WithParamInterface<Params> {
 public:
  UnaffiliatedArcAllowedTest()
      : affiliated_account_id_(
            AccountId::FromUserEmailGaiaId(kAffiliatedUserEmail,
                                           kAffiliatedUserGaiaId)) {
    set_exit_when_last_browser_closes(false);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    DevicePolicyCrosBrowserTest::SetUpCommandLine(command_line);
    arc::SetArcAvailableCommandLineForTesting(command_line);
    affiliation_test_helper::AppendCommandLineSwitchesForLoginManager(
        command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    DevicePolicyCrosBrowserTest::SetUpInProcessBrowserTestFixture();
    UserPolicyBuilder user_policy;
    DevicePolicyCrosTestHelper test_helper;

    std::set<std::string> device_affiliation_ids;
    device_affiliation_ids.insert(kAffiliationID);
    affiliation_test_helper::SetDeviceAffiliationID(
        &test_helper, session_manager_client(), device_affiliation_ids);

    std::set<std::string> user_affiliation_ids;
    if (GetParam().affiliated_)
      user_affiliation_ids.insert(kAffiliationID);
    else
      user_affiliation_ids.insert(kAnotherAffiliationID);

    affiliation_test_helper::SetUserAffiliationIDs(
        &user_policy, session_manager_client(), affiliated_account_id_,
        user_affiliation_ids);
  }

  void TearDownOnMainThread() override {
    // If the login display is still showing, exit gracefully.
    if (chromeos::LoginDisplayHost::default_host()) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(&chrome::AttemptExit));
      content::RunMessageLoop();
    }
    arc::ArcSessionManager::Get()->Shutdown();
    DevicePolicyCrosBrowserTest::TearDownOnMainThread();
  }

 protected:
  void SetPolicy(bool allowed) {
    em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
    proto.mutable_unaffiliated_arc_allowed()->set_unaffiliated_arc_allowed(
        allowed);
    RefreshPolicyAndWaitUntilDeviceSettingsUpdated();
  }

  void RefreshPolicyAndWaitUntilDeviceSettingsUpdated() {
    base::RunLoop run_loop;
    std::unique_ptr<chromeos::CrosSettings::ObserverSubscription> observer =
        chromeos::CrosSettings::Get()->AddSettingsObserver(
            chromeos::kUnaffiliatedArcAllowed, run_loop.QuitClosure());
    RefreshDevicePolicy();
    run_loop.Run();
  }

  const AccountId affiliated_account_id_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UnaffiliatedArcAllowedTest);
};

IN_PROC_BROWSER_TEST_P(UnaffiliatedArcAllowedTest, PRE_ProfileTest) {
  affiliation_test_helper::PreLoginUser(affiliated_account_id_);
}

IN_PROC_BROWSER_TEST_P(UnaffiliatedArcAllowedTest, ProfileTest) {
  affiliation_test_helper::LoginUser(affiliated_account_id_);
  const user_manager::User* user =
      user_manager::UserManager::Get()->FindUser(affiliated_account_id_);
  const Profile* profile =
      chromeos::ProfileHelper::Get()->GetProfileByUser(user);
  const bool affiliated = GetParam().affiliated_;

  EXPECT_EQ(affiliated, user->IsAffiliated());
  EXPECT_TRUE(arc::IsArcAllowedForProfile(profile))
      << "Policy UnaffiliatedArcAllowed is unset, "
      << "expected ARC to be allowed for " << (affiliated ? "" : "un")
      << "affiliated users.";
  SetPolicy(false);
  arc::ResetArcAllowedCheckForTesting(profile);
  EXPECT_EQ(affiliated, arc::IsArcAllowedForProfile(profile))
      << "Policy UnaffiliatedArcAllowed is false, "
      << "expected ARC to be " << (affiliated ? "" : "dis") << "allowed "
      << "for " << (affiliated ? "" : "un") << "affiliated users.";
  SetPolicy(true);
  arc::ResetArcAllowedCheckForTesting(profile);
  EXPECT_TRUE(arc::IsArcAllowedForProfile(profile))
      << "Policy UnaffiliatedArcAllowed is true, "
      << "expected ARC to be allowed for " << (affiliated ? "" : "un")
      << "affiliated users.";
}

INSTANTIATE_TEST_CASE_P(Blub,
                        UnaffiliatedArcAllowedTest,
                        ::testing::Values(Params(true), Params(false)));
}  // namespace policy
