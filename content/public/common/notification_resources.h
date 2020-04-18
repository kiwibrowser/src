// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_NOTIFICATION_RESOURCES_H_
#define CONTENT_PUBLIC_COMMON_NOTIFICATION_RESOURCES_H_

#include <vector>

#include "content/common/content_export.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {

// Structure to hold the resources associated with a Web Notification.
struct CONTENT_EXPORT NotificationResources {
  NotificationResources();
  NotificationResources(const NotificationResources& other);
  ~NotificationResources();

  // Image for the notification. The bitmap may be empty if the developer did
  // not provide an image, or fetching of the image failed.
  SkBitmap image;

  // Main icon for the notification. The bitmap may be empty if the developer
  // did not provide an icon, or fetching of the icon failed.
  SkBitmap notification_icon;

  // Badge for the notification. The bitmap may be empty if the developer
  // did not provide a badge, or fetching of the badge failed.
  SkBitmap badge;

  // Icons for the actions. A bitmap may be empty if the developer did not
  // provide an icon, or fetching of the icon failed.
  std::vector<SkBitmap> action_icons;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_NOTIFICATION_RESOURCES_H_
