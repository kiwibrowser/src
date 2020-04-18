// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_ui_manager.h"

#include <memory>
#include <utility>

#include "chrome/browser/notifications/message_center_notification_manager.h"
#include "ui/message_center/message_center.h"

// static
NotificationUIManager* NotificationUIManager::Create() {
  // If there's no MessageCenter, there should be no NotificationUIManager to
  // manage it.
  if (!message_center::MessageCenter::Get())
    return nullptr;

  return new MessageCenterNotificationManager();
}
