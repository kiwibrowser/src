// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_ACTION_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_ACTION_H_

#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"

namespace blink {

// Structure representing the data associated with a Web Notification action.
struct WebNotificationAction {
  // Corresponds to NotificationActionType.
  enum Type { kButton = 0, kText };

  Type type;
  WebString action;
  WebString title;
  WebURL icon;
  WebString placeholder;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_ACTION_H_
