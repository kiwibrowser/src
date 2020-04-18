// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/session/test_session_controller_client.h"

#include <algorithm>
#include <string>

#include "ash/login_status.h"
#include "ash/public/cpp/session_types.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "components/account_id/account_id.h"
#include "components/prefs/testing_pref_service.h"
#include "components/session_manager/session_manager_types.h"
#include "components/user_manager/user_type.h"

namespace ash {

namespace {

bool g_provide_signin_pref_service = true;

// Returns the "canonicalized" email from a given |email| address. Note
// production code should use gaia::CanonicalizeEmail. This is used in tests
// without introducing dependency on google_api.
std::string GetUserIdFromEmail(const std::string& email) {
  std::string user_id = email;
  std::transform(user_id.begin(), user_id.end(), user_id.begin(), ::tolower);
  return user_id;
}

}  // namespace

// static
void TestSessionControllerClient::DisableAutomaticallyProvideSigninPref() {
  g_provide_signin_pref_service = false;
}

TestSessionControllerClient::TestSessionControllerClient(
    SessionController* controller)
    : controller_(controller), binding_(this) {
  DCHECK(controller_);
  Reset();
}

TestSessionControllerClient::~TestSessionControllerClient() = default;

void TestSessionControllerClient::InitializeAndBind() {
  session_info_->can_lock_screen = controller_->CanLockScreen();
  session_info_->should_lock_screen_automatically =
      controller_->ShouldLockScreenAutomatically();
  session_info_->add_user_session_policy = controller_->GetAddUserPolicy();
  session_info_->state = controller_->GetSessionState();

  ash::mojom::SessionControllerClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  controller_->SetClient(std::move(client));
}

void TestSessionControllerClient::Reset() {
  session_info_ = mojom::SessionInfo::New();
  session_info_->can_lock_screen = true;
  session_info_->should_lock_screen_automatically = false;
  session_info_->add_user_session_policy = AddUserSessionPolicy::ALLOWED;
  session_info_->state = session_manager::SessionState::LOGIN_PRIMARY;

  controller_->ClearUserSessionsForTest();
  controller_->SetSessionInfo(session_info_->Clone());

  if (g_provide_signin_pref_service &&
      !controller_->GetSigninScreenPrefService()) {
    auto pref_service = std::make_unique<TestingPrefServiceSimple>();
    Shell::RegisterSigninProfilePrefs(pref_service->registry(),
                                      true /* for_test */);
    controller_->SetSigninScreenPrefServiceForTest(std::move(pref_service));
  }
}

void TestSessionControllerClient::SetCanLockScreen(bool can_lock) {
  session_info_->can_lock_screen = can_lock;
  controller_->SetSessionInfo(session_info_->Clone());
}

void TestSessionControllerClient::SetShouldLockScreenAutomatically(
    bool should_lock) {
  session_info_->should_lock_screen_automatically = should_lock;
  controller_->SetSessionInfo(session_info_->Clone());
}

void TestSessionControllerClient::SetAddUserSessionPolicy(
    AddUserSessionPolicy policy) {
  session_info_->add_user_session_policy = policy;
  controller_->SetSessionInfo(session_info_->Clone());
}

void TestSessionControllerClient::SetSessionState(
    session_manager::SessionState state) {
  session_info_->state = state;
  controller_->SetSessionInfo(session_info_->Clone());
}

void TestSessionControllerClient::SetIsRunningInAppMode(bool app_mode) {
  session_info_->is_running_in_app_mode = app_mode;
  controller_->SetSessionInfo(session_info_->Clone());
}

void TestSessionControllerClient::CreatePredefinedUserSessions(int count) {
  DCHECK_GT(count, 0);

  // Resets the controller's state.
  Reset();

  // Adds user sessions with numbered emails if more are needed.
  for (int numbered_user_index = 0; numbered_user_index < count;
       ++numbered_user_index) {
    AddUserSession(base::StringPrintf("user%d@tray", numbered_user_index));
  }

  // Sets the first user as active.
  SwitchActiveUser(controller_->GetUserSession(0)->user_info->account_id);

  // Updates session state after adding user sessions.
  SetSessionState(session_manager::SessionState::ACTIVE);
}

void TestSessionControllerClient::AddUserSession(
    const std::string& display_email,
    user_manager::UserType user_type,
    bool enable_settings,
    bool provide_pref_service,
    bool is_new_profile,
    const std::string& service_user_id) {
  auto account_id = AccountId::FromUserEmail(GetUserIdFromEmail(display_email));
  mojom::UserSessionPtr session = mojom::UserSession::New();
  session->session_id = ++fake_session_id_;
  session->user_info = mojom::UserInfo::New();
  session->user_info->avatar = mojom::UserAvatar::New();
  session->user_info->type = user_type;
  session->user_info->account_id = account_id;
  session->user_info->service_user_id = service_user_id;
  session->user_info->display_name = "Über tray Über tray Über tray Über tray";
  session->user_info->display_email = display_email;
  session->user_info->is_ephemeral = false;
  session->user_info->is_new_profile = is_new_profile;
  session->should_enable_settings = enable_settings;
  session->should_show_notification_tray = true;
  controller_->UpdateUserSession(std::move(session));

  if (provide_pref_service &&
      !controller_->GetUserPrefServiceForUser(account_id)) {
    ProvidePrefServiceForUser(account_id);
  }
}

void TestSessionControllerClient::ProvidePrefServiceForUser(
    const AccountId& account_id) {
  DCHECK(!controller_->GetUserPrefServiceForUser(account_id));

  auto pref_service = std::make_unique<TestingPrefServiceSimple>();
  Shell::RegisterUserProfilePrefs(pref_service->registry(),
                                  true /* for_test */);
  controller_->ProvideUserPrefServiceForTest(account_id,
                                             std::move(pref_service));
}

void TestSessionControllerClient::UnlockScreen() {
  SetSessionState(session_manager::SessionState::ACTIVE);
}

void TestSessionControllerClient::RequestLockScreen() {
  SetSessionState(session_manager::SessionState::LOCKED);
}

void TestSessionControllerClient::RequestSignOut() {
  Reset();
}

void TestSessionControllerClient::SwitchActiveUser(
    const AccountId& account_id) {
  std::vector<uint32_t> session_order;
  session_order.reserve(controller_->GetUserSessions().size());

  for (const auto& user_session : controller_->GetUserSessions()) {
    if (user_session->user_info->account_id == account_id) {
      session_order.insert(session_order.begin(), user_session->session_id);
    } else {
      session_order.push_back(user_session->session_id);
    }
  }

  controller_->SetUserSessionOrder(session_order);
}

void TestSessionControllerClient::CycleActiveUser(
    CycleUserDirection direction) {
  DCHECK_GT(controller_->NumberOfLoggedInUsers(), 0);

  const std::vector<mojom::UserSessionPtr>& sessions =
      controller_->GetUserSessions();
  const size_t session_count = sessions.size();

  // The following code depends on that |fake_session_id_| is used to generate
  // session ids.
  uint32_t session_id = sessions[0]->session_id;
  if (direction == CycleUserDirection::NEXT) {
    ++session_id;
  } else if (direction == CycleUserDirection::PREVIOUS) {
    DCHECK_GT(session_id, 0u);
    --session_id;
  } else {
    LOG(ERROR) << "Invalid cycle user direction="
               << static_cast<int>(direction);
    return;
  }

  // Valid range of an session id is [1, session_count].
  if (session_id < 1u)
    session_id = session_count;
  if (session_id > session_count)
    session_id = 1u;

  // Maps session id to AccountId and call SwitchActiveUser.
  auto it = std::find_if(sessions.begin(), sessions.end(),
                         [session_id](const mojom::UserSessionPtr& session) {
                           return session && session->session_id == session_id;
                         });
  if (it == sessions.end()) {
    NOTREACHED();
    return;
  }

  SwitchActiveUser((*it)->user_info->account_id);
}

void TestSessionControllerClient::ShowMultiProfileLogin() {}

}  // namespace ash
