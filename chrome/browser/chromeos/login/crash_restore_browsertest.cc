// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/chromeos/login/session/user_session_manager_test_api.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/session_manager/core/session_manager.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

// Use consumer.example.com to keep policy code out of the tests.
constexpr char kUserId1[] = "user1@consumer.example.com";
constexpr char kUserId2[] = "user2@consumer.example.com";
constexpr char kUserId3[] = "user3@consumer.example.com";

}  // namespace

class CrashRestoreSimpleTest : public InProcessBrowserTest {
 protected:
  CrashRestoreSimpleTest() {}

  ~CrashRestoreSimpleTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(switches::kLoginUser, cryptohome_id1_.id());
    command_line->AppendSwitchASCII(
        switches::kLoginProfile,
        CryptohomeClient::GetStubSanitizedUsername(cryptohome_id1_));
  }

  void SetUpInProcessBrowserTestFixture() override {
    // Redirect session_manager DBus calls to FakeSessionManagerClient.
    session_manager_client_ = new FakeSessionManagerClient;
    chromeos::DBusThreadManager::GetSetterForTesting()->SetSessionManagerClient(
        std::unique_ptr<SessionManagerClient>(session_manager_client_));
    session_manager_client_->StartSession(cryptohome_id1_);
  }

  FakeSessionManagerClient* session_manager_client_;
  const AccountId account_id1_ = AccountId::FromUserEmail(kUserId1);
  const AccountId account_id2_ = AccountId::FromUserEmail(kUserId2);
  const AccountId account_id3_ = AccountId::FromUserEmail(kUserId3);
  const cryptohome::Identification cryptohome_id1_ =
      cryptohome::Identification(account_id1_);
  const cryptohome::Identification cryptohome_id2_ =
      cryptohome::Identification(account_id2_);
  const cryptohome::Identification cryptohome_id3_ =
      cryptohome::Identification(account_id3_);
};

IN_PROC_BROWSER_TEST_F(CrashRestoreSimpleTest, RestoreSessionForOneUser) {
  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  user_manager::User* user = user_manager->GetActiveUser();
  ASSERT_TRUE(user);
  EXPECT_EQ(account_id1_, user->GetAccountId());
  EXPECT_EQ(CryptohomeClient::GetStubSanitizedUsername(cryptohome_id1_),
            user->username_hash());
  EXPECT_EQ(1UL, user_manager->GetLoggedInUsers().size());

  auto* session_manager = session_manager::SessionManager::Get();
  EXPECT_EQ(session_manager::SessionState::ACTIVE,
            session_manager->session_state());
  EXPECT_EQ(1u, session_manager->sessions().size());
}

// Observer that keeps track of user sessions restore event.
class UserSessionRestoreObserver : public UserSessionStateObserver {
 public:
  UserSessionRestoreObserver()
      : running_loop_(false),
        user_sessions_restored_(
            UserSessionManager::GetInstance()->UserSessionsRestored()) {
    if (!user_sessions_restored_)
      UserSessionManager::GetInstance()->AddSessionStateObserver(this);
  }
  ~UserSessionRestoreObserver() override {}

  void PendingUserSessionsRestoreFinished() override {
    user_sessions_restored_ = true;
    UserSessionManager::GetInstance()->RemoveSessionStateObserver(this);
    if (!running_loop_)
      return;

    message_loop_runner_->Quit();
    running_loop_ = false;
  }

  // Wait until the user sessions are restored. If that happened between the
  // construction of this object and this call or even before it was created
  // then it returns immediately.
  void Wait() {
    if (user_sessions_restored_)
      return;

    running_loop_ = true;
    message_loop_runner_ = new content::MessageLoopRunner();
    message_loop_runner_->Run();
  }

 private:
  bool running_loop_;
  bool user_sessions_restored_;
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(UserSessionRestoreObserver);
};

class CrashRestoreComplexTest : public CrashRestoreSimpleTest {
 protected:
  CrashRestoreComplexTest() {}
  ~CrashRestoreComplexTest() override {}

  bool SetUpUserDataDirectory() override {
    RegisterUsers();
    CreateUserProfiles();
    return true;
  }

  void SetUpInProcessBrowserTestFixture() override {
    CrashRestoreSimpleTest::SetUpInProcessBrowserTestFixture();
    session_manager_client_->StartSession(cryptohome_id2_);
    session_manager_client_->StartSession(cryptohome_id3_);
  }

  // Register test users so that UserManager knows them and make kUserId3 as the
  // last active user.
  void RegisterUsers() {
    base::DictionaryValue local_state;

    const char* kTestUserIds[] = {kUserId1, kUserId2, kUserId3};

    auto users_list = std::make_unique<base::ListValue>();
    for (auto* user_id : kTestUserIds)
      users_list->AppendString(user_id);

    local_state.SetList("LoggedInUsers", std::move(users_list));
    local_state.SetString("LastActiveUser", kUserId3);

    auto known_users_list = std::make_unique<base::ListValue>();
    int gaia_id = 10000;
    for (auto* user_id : kTestUserIds) {
      auto user_dict = std::make_unique<base::DictionaryValue>();
      user_dict->SetString("account_type", "google");
      user_dict->SetString("email", user_id);
      user_dict->SetString("gaia_id", base::IntToString(gaia_id++));
      known_users_list->Append(std::move(user_dict));
    }
    local_state.SetList("KnownUsers", std::move(known_users_list));

    std::string local_state_json;
    ASSERT_TRUE(base::JSONWriter::Write(local_state, &local_state_json));

    base::FilePath local_state_file;
    ASSERT_TRUE(
        base::PathService::Get(chrome::DIR_USER_DATA, &local_state_file));
    local_state_file = local_state_file.Append(chrome::kLocalStateFilename);
    ASSERT_NE(-1, base::WriteFile(local_state_file, local_state_json.data(),
                                  local_state_json.size()));
  }

  // Creates user profiles with open user sessions to simulate crashes.
  void CreateUserProfiles() {
    base::DictionaryValue prefs;
    prefs.SetString(prefs::kSessionExitType, "Crashed");
    std::string prefs_json;
    ASSERT_TRUE(base::JSONWriter::Write(prefs, &prefs_json));

    base::FilePath user_data_dir;
    ASSERT_TRUE(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));

    const char* kTestUserIds[] = {kUserId1, kUserId2, kUserId3};
    for (auto* user_id : kTestUserIds) {
      const std::string user_id_hash =
          ProfileHelper::GetUserIdHashByUserIdForTesting(user_id);
      const base::FilePath user_profile_path =
          user_data_dir.Append(ProfileHelper::GetUserProfileDir(user_id_hash));
      ASSERT_TRUE(base::CreateDirectory(user_profile_path));

      ASSERT_NE(-1, base::WriteFile(user_profile_path.Append("Preferences"),
                                    prefs_json.data(), prefs_json.size()));
    }
  }
};

IN_PROC_BROWSER_TEST_F(CrashRestoreComplexTest, RestoreSessionForThreeUsers) {
  {
    UserSessionRestoreObserver restore_observer;
    restore_observer.Wait();
  }

  chromeos::test::UserSessionManagerTestApi session_manager_test_api(
      chromeos::UserSessionManager::GetInstance());
  session_manager_test_api.SetShouldObtainTokenHandleInTests(false);

  DCHECK(UserSessionManager::GetInstance()->UserSessionsRestored());

  // User that is last in the user sessions map becomes active. This behavior
  // will become better defined once each user gets a separate user desktop.
  user_manager::UserManager* user_manager = user_manager::UserManager::Get();
  user_manager::User* user = user_manager->GetActiveUser();
  ASSERT_TRUE(user);
  EXPECT_EQ(account_id3_, user->GetAccountId());
  EXPECT_EQ(CryptohomeClient::GetStubSanitizedUsername(cryptohome_id3_),
            user->username_hash());
  const user_manager::UserList& users = user_manager->GetLRULoggedInUsers();
  ASSERT_EQ(3UL, users.size());

  // User that becomes active moves to the beginning of the list.
  EXPECT_EQ(account_id3_, users[0]->GetAccountId());
  EXPECT_EQ(CryptohomeClient::GetStubSanitizedUsername(cryptohome_id3_),
            users[0]->username_hash());
  EXPECT_EQ(account_id1_, users[1]->GetAccountId());
  EXPECT_EQ(CryptohomeClient::GetStubSanitizedUsername(cryptohome_id1_),
            users[1]->username_hash());
  EXPECT_EQ(account_id2_, users[2]->GetAccountId());
  EXPECT_EQ(CryptohomeClient::GetStubSanitizedUsername(cryptohome_id2_),
            users[2]->username_hash());

  auto* session_manager = session_manager::SessionManager::Get();
  EXPECT_EQ(session_manager::SessionState::ACTIVE,
            session_manager->session_state());
  EXPECT_EQ(3u, session_manager->sessions().size());
  EXPECT_EQ(session_manager->sessions()[0].user_account_id, account_id1_);
  EXPECT_EQ(session_manager->sessions()[1].user_account_id, account_id2_);
  EXPECT_EQ(session_manager->sessions()[2].user_account_id, account_id3_);
}

}  // namespace chromeos
