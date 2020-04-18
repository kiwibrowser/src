// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/platform_notification_data.h"

namespace content {

PlatformNotificationAction::PlatformNotificationAction() {}

PlatformNotificationAction::PlatformNotificationAction(
    const PlatformNotificationAction& other) = default;

PlatformNotificationAction::~PlatformNotificationAction() {}

PlatformNotificationData::PlatformNotificationData() {}

PlatformNotificationData::PlatformNotificationData(
    const PlatformNotificationData& other) = default;

PlatformNotificationData::~PlatformNotificationData() {}

}  // namespace content
