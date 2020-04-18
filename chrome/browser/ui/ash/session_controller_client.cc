// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/session_controller_client.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/session_types.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/ui/user_adding_screen.h"
#include "chrome/browser/chromeos/login/user_flow.h"
#include "chrome/browser/chromeos/login/users/chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/multi_profile_user_controller.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/pref_names.h"
#include "chromeos/assistant/buildflags.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/session_manager/core/session_manager.h"
#include "components/user_manager/user_type.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/notification_service.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/equals_traits.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/chromeos/resources/grit/ui_chromeos_resources.h"
#include "ui/gfx/image/image_skia.h"

#if BUILDFLAG(ENABLE_CROS_ASSISTANT)
#include "chrome/browser/ui/ash/assistant/assistant_client.h"
#endif

using session_manager::Session;
using session_manager::SessionManager;
using session_manager::SessionState;
using user_manager::User;
using user_manager::UserList;
using user_manager::UserManager;

namespace {

// The minimum session length limit that can be set.
const int kSessionLengthLimitMinMs = 30 * 1000;  // 30 seconds.

// The maximum session length limit that can be set.
const int kSessionLengthLimitMaxMs = 24 * 60 * 60 * 1000;  // 24 hours.

SessionControllerClient* g_session_controller_client_instance = nullptr;

// Returns the session id of a given user or 0 if user has no session.
uint32_t GetSessionId(const User& user) {
  const AccountId& account_id = user.GetAccountId();
  for (auto& session : SessionManager::Get()->sessions()) {
    if (session.user_account_id == account_id)
      return session.id;
  }

  return 0u;
}

// Creates a mojom::UserSession for the given user. Returns nullptr if there is
// no user session started for the given user.
ash::mojom::UserSessionPtr UserToUserSession(const User& user) {
  const uint32_t user_session_id = GetSessionId(user);
  if (user_session_id == 0u)
    return nullptr;

  ash::mojom::UserSessionPtr session = ash::mojom::UserSession::New();
  Profile* profile = chromeos::ProfileHelper::Get()->GetProfileByUser(&user);
  session->session_id = user_session_id;
  session->user_info = ash::mojom::UserInfo::New();
  session->user_info->type = user.GetType();
  session->user_info->account_id = user.GetAccountId();
  session->user_info->display_name = base::UTF16ToUTF8(user.display_name());
  session->user_info->display_email = user.display_email();
  session->user_info->is_ephemeral =
      UserManager::Get()->IsUserNonCryptohomeDataEphemeral(user.GetAccountId());
  if (profile) {
    session->user_info->service_user_id =
        content::BrowserContext::GetServiceUserIdFor(profile);
    session->user_info->is_new_profile = profile->IsNewProfile();
  }

  session->user_info->avatar = ash::mojom::UserAvatar::New();
  session->user_info->avatar->image = user.GetImage();
  if (session->user_info->avatar->image.isNull()) {
    session->user_info->avatar->image =
        *ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
            IDR_LOGIN_DEFAULT_USER);
  }

  if (user.IsSupervised()) {
    if (profile) {
      SupervisedUserService* service =
          SupervisedUserServiceFactory::GetForProfile(profile);
      session->custodian_email = service->GetCustodianEmailAddress();
      session->second_custodian_email =
          service->GetSecondCustodianEmailAddress();
    }
  }

  chromeos::UserFlow* const user_flow =
      chromeos::ChromeUserManager::Get()->GetUserFlow(user.GetAccountId());
  session->should_enable_settings = user_flow->ShouldEnableSettings();
  session->should_show_notification_tray =
      user_flow->ShouldShowNotificationTray();

  return session;
}

void DoSwitchUser(const AccountId& account_id, bool switch_user) {
  if (switch_user)
    UserManager::Get()->SwitchActiveUser(account_id);
}

// Callback for the dialog that warns the user about multi-profile, which has
// a "never show again" checkbox.
void OnAcceptMultiprofilesIntroDialog(bool accept, bool never_show_again) {
  if (!accept)
    return;

  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  prefs->SetBoolean(prefs::kMultiProfileNeverShowIntro, never_show_again);
  chromeos::UserAddingScreen::Get()->Start();
}

}  // namespace

namespace mojo {

// When comparing two mojom::UserSession objects we need to decide if the avatar
// images are changed. Consider them equal if they have the same storage rather
// than comparing the backing pixels.
template <>
struct EqualsTraits<gfx::ImageSkia> {
  static bool Equals(const gfx::ImageSkia& a, const gfx::ImageSkia& b) {
    return a.BackedBySameObjectAs(b);
  }
};

}  // namespace mojo

SessionControllerClient::SessionControllerClient()
    : binding_(this), weak_ptr_factory_(this) {
  SessionManager::Get()->AddObserver(this);
  UserManager::Get()->AddSessionStateObserver(this);
  UserManager::Get()->AddObserver(this);

  registrar_.Add(this, chrome::NOTIFICATION_APP_TERMINATING,
                 content::NotificationService::AllSources());
  registrar_.Add(this, chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED,
                 content::NotificationService::AllSources());

  local_state_registrar_ = std::make_unique<PrefChangeRegistrar>();
  local_state_registrar_->Init(g_browser_process->local_state());
  local_state_registrar_->Add(
      prefs::kSessionStartTime,
      base::Bind(&SessionControllerClient::SendSessionLengthLimit,
                 base::Unretained(this)));
  local_state_registrar_->Add(
      prefs::kSessionLengthLimit,
      base::Bind(&SessionControllerClient::SendSessionLengthLimit,
                 base::Unretained(this)));
  chromeos::DeviceSettingsService::Get()
      ->device_off_hours_controller()
      ->AddObserver(this);
  DCHECK(!g_session_controller_client_instance);
  g_session_controller_client_instance = this;
}

SessionControllerClient::~SessionControllerClient() {
  DCHECK_EQ(this, g_session_controller_client_instance);
  g_session_controller_client_instance = nullptr;

  if (supervised_user_profile_) {
    SupervisedUserServiceFactory::GetForProfile(supervised_user_profile_)
        ->RemoveObserver(this);
  }

  SessionManager::Get()->RemoveObserver(this);
  UserManager::Get()->RemoveObserver(this);
  UserManager::Get()->RemoveSessionStateObserver(this);
  chromeos::DeviceSettingsService::Get()
      ->device_off_hours_controller()
      ->RemoveObserver(this);
}

void SessionControllerClient::Init() {
  ConnectToSessionController();
  ash::mojom::SessionControllerClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  session_controller_->SetClient(std::move(client));
  SendSessionInfoIfChanged();
  SendSessionLengthLimit();
  // User sessions and their order will be sent via UserSessionStateObserver
  // even for crash-n-restart.
}

// static
SessionControllerClient* SessionControllerClient::Get() {
  return g_session_controller_client_instance;
}

void SessionControllerClient::PrepareForLock(base::OnceClosure callback) {
  session_controller_->PrepareForLock(std::move(callback));
}

void SessionControllerClient::StartLock(StartLockCallback callback) {
  session_controller_->StartLock(std::move(callback));
}

void SessionControllerClient::NotifyChromeLockAnimationsComplete() {
  session_controller_->NotifyChromeLockAnimationsComplete();
}

void SessionControllerClient::RunUnlockAnimation(
    base::OnceClosure animation_finished_callback) {
  session_controller_->RunUnlockAnimation(
      std::move(animation_finished_callback));
}

void SessionControllerClient::ShowTeleportWarningDialog(
    base::OnceCallback<void(bool, bool)> on_accept) {
  session_controller_->ShowTeleportWarningDialog(std::move(on_accept));
}

void SessionControllerClient::RequestLockScreen() {
  DoLockScreen();
}

void SessionControllerClient::RequestSignOut() {
  chrome::AttemptUserExit();
}

void SessionControllerClient::SwitchActiveUser(const AccountId& account_id) {
  DoSwitchActiveUser(account_id);
}

void SessionControllerClient::CycleActiveUser(
    ash::CycleUserDirection direction) {
  DoCycleActiveUser(direction);
}

void SessionControllerClient::ShowMultiProfileLogin() {
  if (!IsMultiProfileAvailable())
    return;

  // Only regular non-supervised users could add other users to current session.
  if (UserManager::Get()->GetActiveUser()->GetType() !=
      user_manager::USER_TYPE_REGULAR) {
    return;
  }

  if (UserManager::Get()->GetLoggedInUsers().size() >=
      session_manager::kMaximumNumberOfUserSessions) {
    return;
  }

  // Launch sign in screen to add another user to current session.
  if (!UserManager::Get()->GetUsersAllowedForMultiProfile().empty()) {
    // Don't show the dialog if any logged-in user in the multi-profile session
    // dismissed it.
    bool show_intro = true;
    const user_manager::UserList logged_in_users =
        UserManager::Get()->GetLoggedInUsers();
    for (User* user : logged_in_users) {
      show_intro &=
          !multi_user_util::GetProfileFromAccountId(user->GetAccountId())
               ->GetPrefs()
               ->GetBoolean(prefs::kMultiProfileNeverShowIntro);
      if (!show_intro)
        break;
    }
    if (show_intro) {
      session_controller_->ShowMultiprofilesIntroDialog(
          base::Bind(&OnAcceptMultiprofilesIntroDialog));
    } else {
      chromeos::UserAddingScreen::Get()->Start();
    }
  }
}

// static
bool SessionControllerClient::IsMultiProfileAvailable() {
  if (!profiles::IsMultipleProfilesEnabled() || !UserManager::IsInitialized())
    return false;
  size_t users_logged_in = UserManager::Get()->GetLoggedInUsers().size();
  // Does not include users that are logged in.
  size_t users_available_to_add =
      UserManager::Get()->GetUsersAllowedForMultiProfile().size();
  return (users_logged_in + users_available_to_add) > 1;
}

void SessionControllerClient::ActiveUserChanged(const User* active_user) {
  SendSessionInfoIfChanged();

  // UserAddedToSession is not called for the primary user session so its meta
  // data here needs to be sent to ash before setting user session order.
  // However, ActiveUserChanged happens at different timing for primary user
  // and secondary users. For primary user, it happens before user profile load.
  // For secondary users, it happens after user profile load. This caused
  // confusing down the path. Bail out here to defer the primary user session
  // metadata  sent until it becomes active so that ash side could expect a
  // consistent state.
  // TODO(xiyuan): Get rid of this after http://crbug.com/657149 refactoring.
  if (!primary_user_session_sent_ &&
      UserManager::Get()->GetPrimaryUser() == active_user) {
    return;
  }

  SendUserSessionOrder();
}

void SessionControllerClient::UserAddedToSession(const User* added_user) {
  SendSessionInfoIfChanged();
  SendUserSession(*added_user);
}

void SessionControllerClient::OnUserImageChanged(const User& user) {
  SendUserSession(user);
}

// static
bool SessionControllerClient::CanLockScreen() {
  return !UserManager::Get()->GetUnlockUsers().empty();
}

// static
bool SessionControllerClient::ShouldLockScreenAutomatically() {
  // TODO(xiyuan): Observe ash::prefs::kEnableAutoScreenLock and update ash.
  // Tracked in http://crbug.com/670423
  const UserList logged_in_users = UserManager::Get()->GetLoggedInUsers();
  for (auto* user : logged_in_users) {
    Profile* profile = chromeos::ProfileHelper::Get()->GetProfileByUser(user);
    if (profile &&
        profile->GetPrefs()->GetBoolean(ash::prefs::kEnableAutoScreenLock)) {
      return true;
    }
  }
  return false;
}

// static
ash::AddUserSessionPolicy SessionControllerClient::GetAddUserSessionPolicy() {
  UserManager* const user_manager = UserManager::Get();
  if (user_manager->GetUsersAllowedForMultiProfile().empty())
    return ash::AddUserSessionPolicy::ERROR_NO_ELIGIBLE_USERS;

  if (chromeos::MultiProfileUserController::GetPrimaryUserPolicy() !=
      chromeos::MultiProfileUserController::ALLOWED) {
    return ash::AddUserSessionPolicy::ERROR_NOT_ALLOWED_PRIMARY_USER;
  }

  if (UserManager::Get()->GetLoggedInUsers().size() >=
      session_manager::kMaximumNumberOfUserSessions)
    return ash::AddUserSessionPolicy::ERROR_MAXIMUM_USERS_REACHED;

  return ash::AddUserSessionPolicy::ALLOWED;
}

// static
void SessionControllerClient::DoLockScreen() {
  if (!CanLockScreen())
    return;

  VLOG(1) << "Requesting screen lock from SessionControllerClient";
  chromeos::DBusThreadManager::Get()
      ->GetSessionManagerClient()
      ->RequestLockScreen();
}

// static
void SessionControllerClient::DoSwitchActiveUser(const AccountId& account_id) {
  // Disallow switching to an already active user since that might crash.
  if (account_id == UserManager::Get()->GetActiveUser()->GetAccountId())
    return;

  // |client| may be null in tests.
  SessionControllerClient* client = SessionControllerClient::Get();
  if (client) {
    SessionControllerClient::Get()->session_controller_->CanSwitchActiveUser(
        base::Bind(&DoSwitchUser, account_id));
  } else {
    DoSwitchUser(account_id, true);
  }
}

// static
void SessionControllerClient::DoCycleActiveUser(
    ash::CycleUserDirection direction) {
  const UserList& logged_in_users = UserManager::Get()->GetLoggedInUsers();
  if (logged_in_users.size() <= 1)
    return;

  AccountId account_id = UserManager::Get()->GetActiveUser()->GetAccountId();

  // Get an iterator positioned at the active user.
  auto it = std::find_if(logged_in_users.begin(), logged_in_users.end(),
                         [account_id](const User* user) {
                           return user->GetAccountId() == account_id;
                         });

  // Active user not found.
  if (it == logged_in_users.end())
    return;

  // Get the user's email to select, wrapping to the start/end of the list if
  // necessary.
  if (direction == ash::CycleUserDirection::NEXT) {
    if (++it == logged_in_users.end())
      account_id = (*logged_in_users.begin())->GetAccountId();
    else
      account_id = (*it)->GetAccountId();
  } else if (direction == ash::CycleUserDirection::PREVIOUS) {
    if (it == logged_in_users.begin())
      it = logged_in_users.end();
    account_id = (*(--it))->GetAccountId();
  } else {
    NOTREACHED() << "Invalid direction=" << static_cast<int>(direction);
    return;
  }

  DoSwitchActiveUser(account_id);
}

// static
void SessionControllerClient::FlushForTesting() {
  g_session_controller_client_instance->session_controller_.FlushForTesting();
}

void SessionControllerClient::OnSessionStateChanged() {
  // Sent the primary user metadata and user session order that are deferred
  // from ActiveUserChanged before update session state.
  if (!primary_user_session_sent_ &&
      SessionManager::Get()->session_state() == SessionState::ACTIVE) {
    DCHECK_EQ(UserManager::Get()->GetPrimaryUser(),
              UserManager::Get()->GetActiveUser());
    primary_user_session_sent_ = true;
    SendUserSession(*UserManager::Get()->GetPrimaryUser());
    SendUserSessionOrder();

#if BUILDFLAG(ENABLE_CROS_ASSISTANT)
    // Assistant is initialized only once when primary user logs in.
    if (chromeos::switches::IsAssistantEnabled()) {
      AssistantClient::Get()->MaybeInit(
          content::BrowserContext::GetConnectorFor(
              ProfileManager::GetPrimaryUserProfile()));
    }
#endif
  }

  SendSessionInfoIfChanged();
}

void SessionControllerClient::OnCustodianInfoChanged() {
  DCHECK(supervised_user_profile_);
  User* user = chromeos::ProfileHelper::Get()->GetUserByProfile(
      supervised_user_profile_);
  if (user)
    SendUserSession(*user);
}

void SessionControllerClient::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_APP_TERMINATING:
      session_controller_->NotifyChromeTerminating();
      break;
    case chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED: {
      Profile* profile = content::Details<Profile>(details).ptr();
      OnLoginUserProfilePrepared(profile);
      break;
    }
    default:
      NOTREACHED() << "Unexpected notification " << type;
      break;
  }
}

void SessionControllerClient::OnLoginUserProfilePrepared(Profile* profile) {
  const User* user = chromeos::ProfileHelper::Get()->GetUserByProfile(profile);
  DCHECK(user);

  if (profile->IsSupervised()) {
    // There can be only one supervised user per session.
    DCHECK(!supervised_user_profile_);
    supervised_user_profile_ = profile;

    // Watch for changes to supervised user manager/custodians.
    SupervisedUserServiceFactory::GetForProfile(supervised_user_profile_)
        ->AddObserver(this);
  }

  base::Closure session_info_changed_closure =
      base::Bind(&SessionControllerClient::SendSessionInfoIfChanged,
                 weak_ptr_factory_.GetWeakPtr());
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar =
      std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar->Init(profile->GetPrefs());
  pref_change_registrar->Add(ash::prefs::kAllowScreenLock,
                             session_info_changed_closure);
  pref_change_registrar->Add(ash::prefs::kEnableAutoScreenLock,
                             session_info_changed_closure);
  pref_change_registrars_.push_back(std::move(pref_change_registrar));

  // Needed because the user-to-profile mapping isn't available until later,
  // which is needed in UserToUserSession().
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&SessionControllerClient::SendUserSessionForProfile,
                     weak_ptr_factory_.GetWeakPtr(), profile));
}

void SessionControllerClient::OnOffHoursEndTimeChanged() {
  SendSessionLengthLimit();
}

void SessionControllerClient::SendUserSessionForProfile(Profile* profile) {
  DCHECK(profile);
  const User* user = chromeos::ProfileHelper::Get()->GetUserByProfile(profile);
  DCHECK(user);
  SendUserSession(*user);
}

void SessionControllerClient::ConnectToSessionController() {
  // Tests may bind to their own SessionController.
  if (session_controller_)
    return;

  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &session_controller_);
}

void SessionControllerClient::SendSessionInfoIfChanged() {
  SessionManager* const session_manager = SessionManager::Get();

  ash::mojom::SessionInfoPtr info = ash::mojom::SessionInfo::New();
  info->can_lock_screen = CanLockScreen();
  info->should_lock_screen_automatically = ShouldLockScreenAutomatically();
  info->is_running_in_app_mode = chrome::IsRunningInAppMode();
  info->add_user_session_policy = GetAddUserSessionPolicy();
  info->state = session_manager->session_state();

  if (info != last_sent_session_info_) {
    last_sent_session_info_ = info->Clone();
    session_controller_->SetSessionInfo(std::move(info));
  }
}

void SessionControllerClient::SendUserSession(const User& user) {
  ash::mojom::UserSessionPtr user_session = UserToUserSession(user);

  // Bail if the user has no session. Currently the only code path that hits
  // this condition is from OnUserImageChanged when user images are changed
  // on the login screen (e.g. policy change that adds a public session user,
  // or tests that create new users on the login screen).
  if (!user_session)
    return;

  if (user_session != last_sent_user_session_) {
    last_sent_user_session_ = user_session->Clone();
    session_controller_->UpdateUserSession(std::move(user_session));
  }
}

void SessionControllerClient::SendUserSessionOrder() {
  UserManager* const user_manager = UserManager::Get();

  const UserList logged_in_users = user_manager->GetLoggedInUsers();
  std::vector<uint32_t> user_session_ids;
  for (auto* user : user_manager->GetLRULoggedInUsers()) {
    const uint32_t user_session_id = GetSessionId(*user);
    DCHECK_NE(0u, user_session_id);
    user_session_ids.push_back(user_session_id);
  }

  session_controller_->SetUserSessionOrder(user_session_ids);
}

void SessionControllerClient::SendSessionLengthLimit() {
  const PrefService* local_state = local_state_registrar_->prefs();
  base::TimeDelta session_length_limit;
  if (local_state->HasPrefPath(prefs::kSessionLengthLimit)) {
    session_length_limit = base::TimeDelta::FromMilliseconds(
        std::min(std::max(local_state->GetInteger(prefs::kSessionLengthLimit),
                          kSessionLengthLimitMinMs),
                 kSessionLengthLimitMaxMs));
  }
  base::TimeTicks session_start_time;
  if (local_state->HasPrefPath(prefs::kSessionStartTime)) {
    session_start_time = base::TimeTicks::FromInternalValue(
        local_state->GetInt64(prefs::kSessionStartTime));
  }

  policy::off_hours::DeviceOffHoursController* off_hours_controller =
      chromeos::DeviceSettingsService::Get()->device_off_hours_controller();
  base::TimeTicks off_hours_session_end_time;
  // Use "OffHours" end time only if the session will be actually terminated.
  if (off_hours_controller->IsCurrentSessionAllowedOnlyForOffHours())
    off_hours_session_end_time = off_hours_controller->GetOffHoursEndTime();

  // If |session_length_limit| is zero or |session_start_time| is null then
  // "SessionLengthLimit" policy is unset.
  const bool session_length_limit_policy_set =
      !session_length_limit.is_zero() && !session_start_time.is_null();

  // If |off_hours_session_end_time| is null then either "OffHours" policy mode
  // is off or the current session shouldn't be terminated after "OffHours".
  if (off_hours_session_end_time.is_null()) {
    // Send even if both values are zero because enterprise policy could turn
    // both features off in the middle of the session.
    session_controller_->SetSessionLengthLimit(session_length_limit,
                                               session_start_time);
    return;
  }
  if (session_length_limit_policy_set &&
      session_start_time + session_length_limit < off_hours_session_end_time) {
    session_controller_->SetSessionLengthLimit(session_length_limit,
                                               session_start_time);
    return;
  }
  base::TimeTicks off_hours_session_start_time = base::TimeTicks::Now();
  base::TimeDelta off_hours_session_length_limit =
      off_hours_session_end_time - off_hours_session_start_time;
  session_controller_->SetSessionLengthLimit(off_hours_session_length_limit,
                                             off_hours_session_start_time);
}
