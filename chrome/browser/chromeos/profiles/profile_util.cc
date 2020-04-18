// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/profiles/profile_util.h"

#include "base/files/file_path.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/login/login_state.h"

namespace chromeos {

bool IsProfileAssociatedWithGaiaAccount(Profile* profile) {
  if (!chromeos::LoginState::IsInitialized())
    return false;
  if (!chromeos::LoginState::Get()->IsUserGaiaAuthenticated())
    return false;
  if (profile->IsOffTheRecord())
    return false;

  // Using ProfileHelper::GetSigninProfile() here would lead to an infinite loop
  // when this method is called during the creation of the sign-in profile
  // itself. Using ProfileHelper::GetSigninProfileDir() is safe because it does
  // not try to access the sign-in profile.
  if (profile->GetPath() == ProfileHelper::GetSigninProfileDir())
    return false;
  if (profile->GetPath() == ProfileHelper::GetLockScreenAppProfilePath())
    return false;
  return true;
}

}  // namespace chromeos
