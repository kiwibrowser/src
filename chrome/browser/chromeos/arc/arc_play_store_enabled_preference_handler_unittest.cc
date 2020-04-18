// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/arc_play_store_enabled_preference_handler.h"

#include <memory>

#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/test/arc_data_removed_waiter.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_session_runner.h"
#include "components/arc/arc_util.h"
#include "components/arc/test/fake_arc_session.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {
namespace {

class ArcPlayStoreEnabledPreferenceHandlerTest : public testing::Test {
 public:
  ArcPlayStoreEnabledPreferenceHandlerTest()
      : user_manager_enabler_(
            std::make_unique<chromeos::FakeChromeUserManager>()) {}

  void SetUp() override {
    chromeos::DBusThreadManager::GetSetterForTesting()->SetSessionManagerClient(
        std::make_unique<chromeos::FakeSessionManagerClient>());
    chromeos::DBusThreadManager::Initialize();

    SetArcAvailableCommandLineForTesting(
        base::CommandLine::ForCurrentProcess());
    ArcSessionManager::SetUiEnabledForTesting(false);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    TestingProfile::Builder profile_builder;
    profile_builder.SetProfileName("user@gmail.com");
    profile_builder.SetPath(temp_dir_.GetPath().AppendASCII("TestArcProfile"));
    profile_ = profile_builder.Build();

    arc_session_manager_ = std::make_unique<ArcSessionManager>(
        std::make_unique<ArcSessionRunner>(base::Bind(FakeArcSession::Create)));
    preference_handler_ =
        std::make_unique<ArcPlayStoreEnabledPreferenceHandler>(
            profile_.get(), arc_session_manager_.get());
    const AccountId account_id(AccountId::FromUserEmailGaiaId(
        profile()->GetProfileUserName(), "1234567890"));
    GetFakeUserManager()->AddUser(account_id);
    GetFakeUserManager()->LoginUser(account_id);
  }

  void TearDown() override {
    preference_handler_.reset();
    arc_session_manager_.reset();
    profile_.reset();
    chromeos::DBusThreadManager::Shutdown();
  }

  TestingProfile* profile() { return profile_.get(); }
  ArcSessionManager* arc_session_manager() {
    return arc_session_manager_.get();
  }
  ArcPlayStoreEnabledPreferenceHandler* preference_handler() {
    return preference_handler_.get();
  }
  chromeos::FakeChromeUserManager* GetFakeUserManager() {
    return static_cast<chromeos::FakeChromeUserManager*>(
        user_manager::UserManager::Get());
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  user_manager::ScopedUserManager user_manager_enabler_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<ArcSessionManager> arc_session_manager_;
  std::unique_ptr<ArcPlayStoreEnabledPreferenceHandler> preference_handler_;

  DISALLOW_COPY_AND_ASSIGN(ArcPlayStoreEnabledPreferenceHandlerTest);
};

TEST_F(ArcPlayStoreEnabledPreferenceHandlerTest, PrefChangeTriggersService) {
  ASSERT_FALSE(IsArcPlayStoreEnabledForProfile(profile()));
  arc_session_manager()->SetProfile(profile());
  arc_session_manager()->Initialize();
  preference_handler()->Start();

  EXPECT_FALSE(
      profile()->GetPrefs()->GetBoolean(prefs::kArcDataRemoveRequested));
  EXPECT_EQ(ArcSessionManager::State::STOPPED, arc_session_manager()->state());

  SetArcPlayStoreEnabledForProfile(profile(), true);
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(ArcSessionManager::State::NEGOTIATING_TERMS_OF_SERVICE,
            arc_session_manager()->state());

  SetArcPlayStoreEnabledForProfile(profile(), false);

  ArcDataRemovedWaiter().Wait();
  ASSERT_EQ(ArcSessionManager::State::STOPPED, arc_session_manager()->state());
}

TEST_F(ArcPlayStoreEnabledPreferenceHandlerTest,
       PrefChangeTriggersService_Restart) {
  // Sets the Google Play Store preference at beginning.
  SetArcPlayStoreEnabledForProfile(profile(), true);

  arc_session_manager()->SetProfile(profile());
  arc_session_manager()->Initialize();
  preference_handler()->Start();

  // Setting profile initiates a code fetching process.
  ASSERT_EQ(ArcSessionManager::State::NEGOTIATING_TERMS_OF_SERVICE,
            arc_session_manager()->state());
}

TEST_F(ArcPlayStoreEnabledPreferenceHandlerTest, RemoveDataDir_Managed) {
  // Set ARC to be managed and disabled.
  profile()->GetTestingPrefService()->SetManagedPref(
      prefs::kArcEnabled, std::make_unique<base::Value>(false));

  // Starting session manager with prefs::kArcEnabled off in a managed profile
  // does automatically remove Android's data folder.
  arc_session_manager()->SetProfile(profile());
  arc_session_manager()->Initialize();
  preference_handler()->Start();
  EXPECT_TRUE(
      profile()->GetPrefs()->GetBoolean(prefs::kArcDataRemoveRequested));
}

}  // namespace
}  // namespace arc
