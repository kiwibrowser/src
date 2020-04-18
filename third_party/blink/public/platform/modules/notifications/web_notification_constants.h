// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_CONSTANTS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_CONSTANTS_H_

namespace blink {

// TODO(johnme): The maximum number of actions is platform-specific and should
// be indicated by the embedder.

// Maximum number of actions on a Platform Notification.
static constexpr size_t kWebNotificationMaxActions = 2;

// TODO(mvanouwerkerk): Update the notification resource loader to get the
// appropriate image sizes from the embedder.

// The maximum reasonable image size, scaled from dip units to pixels using the
// largest supported scaling factor. TODO(johnme): Check sizes are correct.
static constexpr int kWebNotificationMaxImageWidthPx = 1800;  // 450 dip * 4
static constexpr int kWebNotificationMaxImageHeightPx = 900;  // 225 dip * 4

// The maximum reasonable notification icon size, scaled from dip units to
// pixels using the largest supported scaling factor.
static constexpr int kWebNotificationMaxIconSizePx = 320;  // 80 dip * 4

// The maximum reasonable badge size, scaled from dip units to pixels using the
// largest supported scaling factor.
static constexpr int kWebNotificationMaxBadgeSizePx = 96;  // 24 dip * 4

// The maximum reasonable action icon size, scaled from dip units to
// pixels using the largest supported scaling factor.
static constexpr int kWebNotificationMaxActionIconSizePx = 128;  // 32 dip * 4

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_NOTIFICATIONS_WEB_NOTIFICATION_CONSTANTS_H_
