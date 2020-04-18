// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/idle/idle.h"

#include "base/time/time.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "ui/base/user_activity/user_activity_detector.h"

namespace ui {

void CalculateIdleTime(IdleTimeCallback notify) {
  base::TimeDelta idle_time = base::TimeTicks::Now() -
      ui::UserActivityDetector::Get()->last_activity_time();
  notify.Run(static_cast<int>(idle_time.InSeconds()));
}

bool CheckIdleStateIsLocked() {
  return chromeos::DBusThreadManager::Get()->GetSessionManagerClient()->
      IsScreenLocked();
}

}  // namespace ui
