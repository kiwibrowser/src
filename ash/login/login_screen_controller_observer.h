// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOGIN_LOGIN_SCREEN_CONTROLLER_OBSERVER_H_
#define ASH_LOGIN_LOGIN_SCREEN_CONTROLLER_OBSERVER_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/user_info.mojom.h"

class AccountId;

namespace ash {

// Interface used by LoginScreenController when some events have been received
// from another process, ie, chrome.
class ASH_EXPORT LoginScreenControllerObserver {
 public:
  virtual ~LoginScreenControllerObserver();

  // Called when |avatar| for |account_id| has changed.
  virtual void SetAvatarForUser(const AccountId& account_id,
                                const mojom::UserAvatarPtr& avatar) = 0;

  // Called when focus is leaving a lock screen app window due to tabbing.
  // |reverse| - whether the tab order is reversed.
  virtual void OnFocusLeavingLockScreenApps(bool reverse) = 0;
};

}  // namespace ash

#endif  // ASH_LOGIN_LOGIN_SCREEN_CONTROLLER_OBSERVER_H_
