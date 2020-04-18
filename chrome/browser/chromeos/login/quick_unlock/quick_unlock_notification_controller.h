// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_QUICK_UNLOCK_NOTIFICATION_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_QUICK_UNLOCK_NOTIFICATION_CONTROLLER_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "url/gurl.h"

class Profile;

namespace message_cener {
class Notification;
}

namespace chromeos {
namespace quick_unlock {

// Quick Unlock feature notification controller is responsible for managing the
// new feature notification displayed to the user.
class QuickUnlockNotificationController
    : public message_center::NotificationDelegate,
      public content::NotificationObserver {
 public:
  static QuickUnlockNotificationController* CreateForPin(Profile* profile);
  // Returns true if the notification needs to be displayed for the given
  // |profile|.
  static bool ShouldShowPinNotification(Profile* profile);

  static QuickUnlockNotificationController* CreateForFingerprint(
      Profile* profile);
  static bool ShouldShowFingerprintNotification(Profile* profile);

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  // Parameters that differ between two quick unlock notifications.
  struct NotificationParams {
    NotificationParams();
    ~NotificationParams();

    int title_message_id;
    int body_message_id;
    int icon_id;
    std::string notifier;
    int feature_name_id;
    std::string notification_id;
    GURL url;
    std::string was_shown_pref_id;

   private:
    DISALLOW_COPY_AND_ASSIGN(NotificationParams);
  };

  explicit QuickUnlockNotificationController(Profile* profile);
  ~QuickUnlockNotificationController() override;

  // NotificationDelegate:
  void Close(bool by_user) override;
  void Click(const base::Optional<int>& button_index,
             const base::Optional<base::string16>& reply) override;

  std::unique_ptr<message_center::Notification> CreateNotification();
  void SetNotificationPreferenceWasShown();
  void UnregisterObserver();

  Profile* profile_;
  content::NotificationRegistrar registrar_;
  NotificationParams params_;
  // Function that determines whether this notification should be shown or not.
  base::Callback<bool(Profile*)> should_show_notification_callback_;

  DISALLOW_COPY_AND_ASSIGN(QuickUnlockNotificationController);
};

}  // namespace quick_unlock
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_QUICK_UNLOCK_QUICK_UNLOCK_NOTIFICATION_CONTROLLER_H_
