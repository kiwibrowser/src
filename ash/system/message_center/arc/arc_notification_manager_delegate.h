// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_MANAGER_DELEGATE_H_
#define ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_MANAGER_DELEGATE_H_

#include <string>

#include "base/callback.h"

namespace ash {

// Delegate interface of arc notification code to call into ash code.
class ArcNotificationManagerDelegate {
 public:
  virtual ~ArcNotificationManagerDelegate() = default;

  // Whether the current user session is public session or kiosk.
  virtual bool IsPublicSessionOrKiosk() const = 0;

  // Gets app id by package name.
  using GetAppIdByPackageNameCallback =
      base::OnceCallback<void(const std::string& app_id)>;
  virtual void GetAppIdByPackageName(
      const std::string& package_name,
      GetAppIdByPackageNameCallback callback) = 0;

  // Shows the message center.
  virtual void ShowMessageCenter() = 0;
};

}  // namespace ash

#endif  // ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_MANAGER_DELEGATE_H_
