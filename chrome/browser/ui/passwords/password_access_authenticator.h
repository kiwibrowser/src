// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PASSWORDS_PASSWORD_ACCESS_AUTHENTICATOR_H_
#define CHROME_BROWSER_UI_PASSWORDS_PASSWORD_ACCESS_AUTHENTICATOR_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "chrome/browser/password_manager/reauth_purpose.h"

// This class takes care of reauthentication used for accessing passwords
// through the settings page. It is used on all platforms but iOS and Android
// (see //ios/chrome/browser/ui/settings/reauthentication_module.* for iOS and
// PasswordEntryEditor.java and PasswordReauthenticationFragment.java in
// chrome/android/java/src/org/chromium/chrome/browser/preferences/password/
// for Android).
class PasswordAccessAuthenticator {
 public:
  using ReauthCallback =
      base::RepeatingCallback<bool(password_manager::ReauthPurpose)>;

  // For how long after the last successful authentication a user is considered
  // authenticated without repeating the challenge.
  constexpr static int kAuthValidityPeriodSeconds = 60;

  // |os_reauth_call| is passed to |os_reauth_call_|, see the latter for
  // explanation.
  explicit PasswordAccessAuthenticator(ReauthCallback os_reauth_call);

  ~PasswordAccessAuthenticator();

  // Returns whether the user is able to pass the authentication challenge,
  // which is represented by |os_reauth_call_| returning true. A successful
  // result of |os_reauth_call_| is cached for |kAuthValidityPeriodSeconds|
  // seconds.
  bool EnsureUserIsAuthenticated(password_manager::ReauthPurpose purpose);

  // Presents the reauthentication challenge to the user and returns whether
  // the user passed the challenge. This call is guaranteed to present the
  // challenge to the user.
  bool ForceUserReauthentication(password_manager::ReauthPurpose purpose);

  // Use this in tests to mock the OS-level reauthentication.
  void SetOsReauthCallForTesting(ReauthCallback os_reauth_call);

  // Use this to manipulate time in tests.
  void SetClockForTesting(base::Clock* clock);

 private:
  // The last time the user was successfully authenticated.
  base::Optional<base::Time> last_authentication_time_;

  // Used to measure the time since the last authentication.
  base::Clock* clock_;

  // Used to directly present the authentication challenge (such as the login
  // prompt) to the user.
  ReauthCallback os_reauth_call_;

  DISALLOW_COPY_AND_ASSIGN(PasswordAccessAuthenticator);
};

#endif  // CHROME_BROWSER_UI_PASSWORDS_PASSWORD_ACCESS_AUTHENTICATOR_H_
