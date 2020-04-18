// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_SERVICE_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_SERVICE_OBSERVER_H_

#include "chromeos/components/proximity_auth/screenlock_state.h"

namespace chromeos {

class EasyUnlockServiceObserver {
 public:
  // Invoked when turn-off operation status changes.
  virtual void OnTurnOffOperationStatusChanged() {}

  // Invoked when screenlock state changes.
  virtual void OnScreenlockStateChanged(proximity_auth::ScreenlockState state) {
  }

 protected:
  virtual ~EasyUnlockServiceObserver() {}
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_SERVICE_OBSERVER_H_
