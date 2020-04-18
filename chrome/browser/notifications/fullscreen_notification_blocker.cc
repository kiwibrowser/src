// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/fullscreen_notification_blocker.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/fullscreen.h"
#include "content/public/browser/notification_service.h"
#include "ui/message_center/public/cpp/notifier_id.h"

using message_center::NotifierId;

FullscreenNotificationBlocker::FullscreenNotificationBlocker(
    message_center::MessageCenter* message_center)
    : NotificationBlocker(message_center),
      is_fullscreen_mode_(false) {
  registrar_.Add(this, chrome::NOTIFICATION_FULLSCREEN_CHANGED,
                 content::NotificationService::AllSources());
}

FullscreenNotificationBlocker::~FullscreenNotificationBlocker() {
}

void FullscreenNotificationBlocker::CheckState() {
  bool was_fullscreen_mode = is_fullscreen_mode_;
  is_fullscreen_mode_ = IsFullScreenMode();
  if (is_fullscreen_mode_ != was_fullscreen_mode)
    NotifyBlockingStateChanged();
}

bool FullscreenNotificationBlocker::ShouldShowNotificationAsPopup(
    const message_center::Notification& notification) const {
  bool enabled =
      !is_fullscreen_mode_ || (notification.fullscreen_visibility() !=
                               message_center::FullscreenVisibility::NONE);

  if (enabled && !is_fullscreen_mode_) {
    UMA_HISTOGRAM_ENUMERATION("Notifications.Display_Windowed",
                              notification.notifier_id().type,
                              NotifierId::SIZE);
  }

  return enabled;
}

void FullscreenNotificationBlocker::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_FULLSCREEN_CHANGED, type);
  CheckState();
}
