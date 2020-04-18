// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/persistent_notification_handler.h"

#include "base/callback.h"
#include "base/logging.h"
#include "chrome/browser/notifications/desktop_notification_profile_util.h"
#include "chrome/browser/notifications/platform_notification_service_impl.h"
#include "chrome/browser/profiles/profile.h"

PersistentNotificationHandler::PersistentNotificationHandler() = default;
PersistentNotificationHandler::~PersistentNotificationHandler() = default;

void PersistentNotificationHandler::OnClose(
    Profile* profile,
    const GURL& origin,
    const std::string& notification_id,
    bool by_user,
    base::OnceClosure completed_closure) {
  if (!by_user) {
    std::move(completed_closure).Run();
    return;  // no need to propagate back programmatic close events
  }

  DCHECK(origin.is_valid());

  PlatformNotificationServiceImpl::GetInstance()->OnPersistentNotificationClose(
      profile, notification_id, origin, by_user, std::move(completed_closure));
}

void PersistentNotificationHandler::OnClick(
    Profile* profile,
    const GURL& origin,
    const std::string& notification_id,
    const base::Optional<int>& action_index,
    const base::Optional<base::string16>& reply,
    base::OnceClosure completed_closure) {
  DCHECK(origin.is_valid());

  PlatformNotificationServiceImpl::GetInstance()->OnPersistentNotificationClick(
      profile, notification_id, origin, action_index, reply,
      std::move(completed_closure));
}

void PersistentNotificationHandler::DisableNotifications(Profile* profile,
                                                         const GURL& origin) {
  DesktopNotificationProfileUtil::DenyPermission(profile, origin);
}

void PersistentNotificationHandler::OpenSettings(Profile* profile,
                                                 const GURL& origin) {
  NotificationCommon::OpenNotificationSettings(profile, origin);
}
