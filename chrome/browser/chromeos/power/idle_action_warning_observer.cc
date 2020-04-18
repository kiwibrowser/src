// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/idle_action_warning_observer.h"

#include "base/time/time.h"
#include "chrome/browser/chromeos/power/idle_action_warning_dialog_view.h"
#include "chromeos/dbus/dbus_thread_manager.h"

namespace chromeos {

IdleActionWarningObserver::IdleActionWarningObserver() : warning_dialog_(NULL) {
  DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(this);
}

IdleActionWarningObserver::~IdleActionWarningObserver() {
  DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(this);
  if (warning_dialog_)
    warning_dialog_->CloseDialog();
}

void IdleActionWarningObserver::IdleActionImminent(
    const base::TimeDelta& time_until_idle_action) {
  const base::TimeTicks idle_action_time =
      base::TimeTicks::Now() + time_until_idle_action;
  if (warning_dialog_)
    warning_dialog_->Update(idle_action_time);
  else
    warning_dialog_ = new IdleActionWarningDialogView(idle_action_time);
}

void IdleActionWarningObserver::IdleActionDeferred() {
  if (warning_dialog_)
    warning_dialog_->CloseDialog();
  warning_dialog_ = NULL;
}

}  // namespace chromeos
