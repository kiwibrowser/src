// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_IDLE_ACTION_WARNING_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_IDLE_ACTION_WARNING_OBSERVER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {

class IdleActionWarningDialogView;

// Listens for notifications that the idle action is imminent and shows a
// warning dialog to the user.
class IdleActionWarningObserver : public PowerManagerClient::Observer {
 public:
  IdleActionWarningObserver();
  ~IdleActionWarningObserver() override;

  // PowerManagerClient::Observer:
  void IdleActionImminent(
      const base::TimeDelta& time_until_idle_action) override;
  void IdleActionDeferred() override;

 private:
  IdleActionWarningDialogView* warning_dialog_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(IdleActionWarningObserver);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_IDLE_ACTION_WARNING_OBSERVER_H_
