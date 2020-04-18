// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_promo_util.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/signin_manager.h"
#include "net/base/network_change_notifier.h"

namespace signin {

bool ShouldShowPromo(Profile* profile) {
#if defined(OS_CHROMEOS)
  // There's no need to show the sign in promo on cros since cros users are
  // already logged in.
  return false;
#else

  // Don't bother if we don't have any kind of network connection.
  if (net::NetworkChangeNotifier::IsOffline())
    return false;

  // Don't show for supervised profiles.
  if (profile->IsSupervised())
    return false;

  // Display the signin promo if the user is not signed in.
  SigninManager* signin =
      SigninManagerFactory::GetForProfile(profile->GetOriginalProfile());
  return !signin->AuthInProgress() && signin->IsSigninAllowed() &&
         !signin->IsAuthenticated();
#endif
}

}  // namespace signin
