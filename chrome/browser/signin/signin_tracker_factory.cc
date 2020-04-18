// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/signin_tracker_factory.h"

#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/profile_management_switches.h"

SigninTrackerFactory::SigninTrackerFactory() {}
SigninTrackerFactory::~SigninTrackerFactory() {}

// static
std::unique_ptr<SigninTracker> SigninTrackerFactory::CreateForProfile(
    Profile* profile,
    SigninTracker::Observer* observer) {
  return std::unique_ptr<SigninTracker>(new SigninTracker(
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile),
      SigninManagerFactory::GetForProfile(profile),
      GaiaCookieManagerServiceFactory::GetForProfile(profile), observer));
}
