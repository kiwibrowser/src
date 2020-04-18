// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_FULLSCREEN_NOTIFICATION_BLOCKER_H_
#define CHROME_BROWSER_NOTIFICATIONS_FULLSCREEN_NOTIFICATION_BLOCKER_H_

#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "ui/message_center/notification_blocker.h"

// A notification blocker which checks the fullscreen state. This is not used on
// ChromeOS as ash has its own fullscreen notification blocker.
class FullscreenNotificationBlocker
    : public message_center::NotificationBlocker,
      public content::NotificationObserver {
 public:
  explicit FullscreenNotificationBlocker(
      message_center::MessageCenter* message_center);
  ~FullscreenNotificationBlocker() override;

  // message_center::NotificationBlocker overrides:
  void CheckState() override;
  bool ShouldShowNotificationAsPopup(
      const message_center::Notification& notification) const override;

 private:
  // content::NotificationObserver override.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  bool is_fullscreen_mode_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenNotificationBlocker);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_FULLSCREEN_NOTIFICATION_BLOCKER_H_
