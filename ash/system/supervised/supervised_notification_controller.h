// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SUPERVISED_SUPERVISED_NOTIFICATION_CONTROLLER_H_
#define ASH_SYSTEM_SUPERVISED_SUPERVISED_NOTIFICATION_CONTROLLER_H_

#include <string>

#include "ash/ash_export.h"
#include "ash/session/session_observer.h"
#include "base/strings/string16.h"

namespace ash {

// Controller class to manage supervised user notification.
class ASH_EXPORT SupervisedNotificationController : public SessionObserver {
 public:
  SupervisedNotificationController();
  ~SupervisedNotificationController() override;

  // SessionObserver:
  void OnActiveUserSessionChanged(const AccountId& account_id) override;
  void OnUserSessionAdded(const AccountId& account_id) override;
  void OnUserSessionUpdated(const AccountId& account_id) override;

 private:
  friend class SupervisedNotificationControllerTest;

  static const char kNotificationId[];

  static void CreateOrUpdateNotification();

  std::string custodian_email_;
  std::string second_custodian_email_;

  ScopedSessionObserver scoped_session_observer_{this};

  DISALLOW_COPY_AND_ASSIGN(SupervisedNotificationController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_SUPERVISED_SUPERVISED_NOTIFICATION_CONTROLLER_H_
