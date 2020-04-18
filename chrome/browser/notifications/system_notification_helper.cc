// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/system_notification_helper.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile_manager.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#endif

SystemNotificationHelper* SystemNotificationHelper::GetInstance() {
  return base::Singleton<SystemNotificationHelper>::get();
}

SystemNotificationHelper::SystemNotificationHelper() = default;
SystemNotificationHelper::~SystemNotificationHelper() = default;

void SystemNotificationHelper::Display(
    const message_center::Notification& notification) {
  pending_notifications_[notification.id()] = notification;
  g_browser_process->profile_manager()->CreateProfileAsync(
      GetProfilePath(),
      base::AdaptCallbackForRepeating(
          base::BindOnce(&SystemNotificationHelper::DoDisplayNotification,
                         weak_factory_.GetWeakPtr(), notification.id())),
      base::string16(), std::string(), std::string());
}

void SystemNotificationHelper::Close(const std::string& notification_id) {
  size_t erased = pending_notifications_.erase(notification_id);
  Profile* profile =
      g_browser_process->profile_manager()->GetProfileByPath(GetProfilePath());
  if (!profile)
    return;

  // If the profile has finished loading, we should have already removed the
  // notification from the pending list in DoDisplayNotification().
  DCHECK_EQ(0u, erased);
  NotificationDisplayService::GetForProfile(profile->GetOffTheRecordProfile())
      ->Close(NotificationHandler::Type::TRANSIENT, notification_id);
}

void SystemNotificationHelper::DoDisplayNotification(
    const std::string& notification_id,
    Profile* profile,
    Profile::CreateStatus status) {
  auto iter = pending_notifications_.find(notification_id);
  if (iter == pending_notifications_.end())
    return;

  if (profile) {
    // We use the incognito profile both to match
    // ProfileHelper::GetSigninProfile() and to be sure we don't store anything
    // about it across program restarts.
    NotificationDisplayService::GetForProfile(profile->GetOffTheRecordProfile())
        ->Display(NotificationHandler::Type::TRANSIENT, iter->second);
  }
  pending_notifications_.erase(iter);
}

// static
Profile* SystemNotificationHelper::GetProfileForTesting() {
  return g_browser_process->profile_manager()
      ->GetProfile(GetProfilePath())
      ->GetOffTheRecordProfile();
}

// static
base::FilePath SystemNotificationHelper::GetProfilePath() {
#if defined(OS_CHROMEOS)
  // System notifications (such as those for network state) aren't tied to a
  // particular user and can show up before any user is logged in, so use the
  // signin profile, which is guaranteed to already exist.
  return chromeos::ProfileHelper::GetSigninProfileDir();
#else
  // The "system profile" probably hasn't been loaded yet.
  return g_browser_process->profile_manager()->GetSystemProfilePath();
#endif
}
