// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_reauth.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/components/proximity_auth/screenlock_bridge.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/login/auth/auth_status_consumer.h"
#include "chromeos/login/auth/user_context.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "ui/base/l10n/l10n_util.h"

namespace chromeos {

namespace {

void EndReauthAttempt();

// Performs the actual reauth flow and returns the user context it obtains.
class ReauthHandler : public content::NotificationObserver,
                      public AuthStatusConsumer {
 public:
  explicit ReauthHandler(EasyUnlockReauth::UserContextCallback callback)
      : callback_(callback) {}

  ~ReauthHandler() override {}

  bool Start() {
    ScreenLocker* screen_locker = ScreenLocker::default_screen_locker();
    if (screen_locker && screen_locker->locked()) {
      NOTREACHED();
      return false;
    }

    user_manager::UserManager* user_manager = user_manager::UserManager::Get();
    if (user_manager->GetPrimaryUser() != user_manager->GetActiveUser() ||
        user_manager->GetUnlockUsers().size() != 1) {
      LOG(WARNING) << "Only primary users in non-multiprofile sessions are "
                   << "currently supported for reauth.";
      return false;
    }

    notification_registrar_.Add(this,
                                chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
                                content::NotificationService::AllSources());

    SessionManagerClient* session_manager =
        DBusThreadManager::Get()->GetSessionManagerClient();
    session_manager->RequestLockScreen();
    return true;
  }

  // content::NotificationObserver
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    CHECK(type == chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED);
    bool is_screen_locked = *content::Details<bool>(details).ptr();
    DCHECK(is_screen_locked);
    notification_registrar_.RemoveAll();

    // TODO(tengs): Add an explicit reauth state to the locker and account
    // picker, so we can customize the UI.
    ScreenLocker* screen_locker = ScreenLocker::default_screen_locker();
    screen_locker->SetLoginStatusConsumer(this);

    // Show tooltip explaining reauth.
    proximity_auth::ScreenlockBridge::UserPodCustomIconOptions icon_options;
    icon_options.SetIcon(
        proximity_auth::ScreenlockBridge::USER_POD_CUSTOM_ICON_NONE);
    icon_options.SetTooltip(
        l10n_util::GetStringUTF16(
            IDS_SMART_LOCK_SCREENLOCK_TOOLTIP_HARDLOCK_REAUTH_USER),
        true);

    const user_manager::UserList& lock_users = screen_locker->users();
    DCHECK(lock_users.size() == 1);
    proximity_auth::ScreenlockBridge::Get()
        ->lock_handler()
        ->ShowUserPodCustomIcon(lock_users[0]->GetAccountId(), icon_options);
  }

  // AuthStatusConsumer:
  void OnAuthSuccess(const UserContext& user_context) override {
    DCHECK(base::MessageLoopForUI::IsCurrent());
    callback_.Run(user_context);
    // Schedule deletion.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&EndReauthAttempt));
  }

  void OnAuthFailure(const AuthFailure& error) override {}

 private:
  content::NotificationRegistrar notification_registrar_;
  EasyUnlockReauth::UserContextCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(ReauthHandler);
};

ReauthHandler* g_reauth_handler = NULL;

void EndReauthAttempt() {
  DCHECK(base::MessageLoopForUI::IsCurrent());
  DCHECK(g_reauth_handler);
  delete g_reauth_handler;
  g_reauth_handler = NULL;
}

}  // namespace

// static.
bool EasyUnlockReauth::ReauthForUserContext(
    base::Callback<void(const UserContext&)> callback) {
  DCHECK(base::MessageLoopForUI::IsCurrent());
  if (g_reauth_handler)
    return false;

  g_reauth_handler = new ReauthHandler(callback);
  return g_reauth_handler->Start();
}

}  // namespace chromeos
