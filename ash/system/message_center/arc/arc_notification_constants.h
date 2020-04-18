// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_CONSTANTS_H_
#define ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_CONSTANTS_H_

namespace ash {

// The fallback notifier id for ARC notifications. Used when ArcNotificationItem
// is provided with an empty app id.
constexpr char kDefaultArcNotifierId[] = "ARC_NOTIFICATION";

// The prefix used when ARC generates a notification id, which is used in Chrome
// world, from a notification key, which is used in Android.
constexpr char kArcNotificationIdPrefix[] = "ARC_NOTIFICATION_";

}  // namespace ash

#endif  // ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_CONSTANTS_H_
