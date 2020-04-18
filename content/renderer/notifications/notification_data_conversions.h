// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_NOTIFICATIONS_NOTIFICATION_DATA_CONVERSIONS_H_
#define CONTENT_RENDERER_NOTIFICATIONS_NOTIFICATION_DATA_CONVERSIONS_H_

#include "content/common/content_export.h"
#include "content/public/common/platform_notification_data.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_data.h"

namespace content {

// Converts PlatformNotificationData to Blink WebNotificationData.
CONTENT_EXPORT blink::WebNotificationData ToWebNotificationData(
    const PlatformNotificationData& platform_data);

}  // namespace content

#endif  // CONTENT_RENDERER_NOTIFICATIONS_NOTIFICATION_DATA_CONVERSIONS_H_
