// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SAML_SAML_OFFLINE_SIGNIN_LIMITER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SAML_SAML_OFFLINE_SIGNIN_LIMITER_H_

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chromeos/login/auth/user_context.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

namespace base {
class Clock;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace chromeos {

// Enforces a limit on the length of time for which a user authenticated via
// SAML can use offline authentication against a cached password before being
// forced to go through online authentication against GAIA again.
class SAMLOfflineSigninLimiter : public KeyedService {
 public:
  // Registers preferences.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Called when the user successfully authenticates. |auth_flow| indicates
  // the type of authentication flow that the user went through.
  void SignedIn(UserContext::AuthFlow auth_flow);

  // KeyedService:
  void Shutdown() override;

 private:
  friend class SAMLOfflineSigninLimiterFactory;
  friend class SAMLOfflineSigninLimiterTest;

  // |profile| and |clock| must remain valid until Shutdown() is called. If
  // |clock| is NULL, the shared base::DefaultClock instance will be used.
  SAMLOfflineSigninLimiter(Profile* profile, base::Clock* clock);
  ~SAMLOfflineSigninLimiter() override;

  // Recalculates the amount of time remaining until online login should be
  // forced and sets the |offline_signin_limit_timer_| accordingly. If the limit
  // has expired already, sets the flag enforcing online login immediately.
  void UpdateLimit();

  // Sets the flag enforcing online login. This will cause the user's next login
  // to use online authentication against GAIA.
  void ForceOnlineLogin();

  Profile* profile_;
  base::Clock* clock_;

  PrefChangeRegistrar pref_change_registrar_;

  std::unique_ptr<base::OneShotTimer> offline_signin_limit_timer_;

  DISALLOW_COPY_AND_ASSIGN(SAMLOfflineSigninLimiter);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SAML_SAML_OFFLINE_SIGNIN_LIMITER_H_
