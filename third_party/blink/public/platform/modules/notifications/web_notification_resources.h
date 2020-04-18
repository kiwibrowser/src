// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_RESOURCES_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_RESOURCES_H_

#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

// Structure representing the resources associated with a Web Notification.
struct WebNotificationResources {
  // Content image for the notification. The bitmap may be empty if the
  // developer did not provide an image, or fetching of the image failed.
  SkBitmap image;

  // Main icon for the notification. The bitmap may be empty if the developer
  // did not provide an icon, or fetching of the icon failed.
  SkBitmap icon;

  // Badge for the notification. The bitmap may be empty if the developer
  // did not provide a badge, or fetching of the badge failed.
  SkBitmap badge;

  // Icons for the actions. A bitmap may be empty if the developer did not
  // provide an icon, or fetching of the icon failed.
  WebVector<SkBitmap> action_icons;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_RESOURCES_H_
