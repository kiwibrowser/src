// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_REAUTH_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_REAUTH_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"

namespace chromeos {

class UserContext;

// Responsible for locking the screen and reauthenticating the user so we can
// create new cryptohome keys for passwordless sign-in.
class EasyUnlockReauth {
 public:
  typedef base::Callback<void(const UserContext&)> UserContextCallback;

  // Launches the reauth screen to get the user context. If the screen fails
  // for some reason, then this function will return false.
  static bool ReauthForUserContext(UserContextCallback callback);

  DISALLOW_IMPLICIT_CONSTRUCTORS(EasyUnlockReauth);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_EASY_UNLOCK_REAUTH_H_
