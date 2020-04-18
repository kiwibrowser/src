// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_UNIFIED_CONSENT_HELPER_H_
#define CHROME_BROWSER_SIGNIN_UNIFIED_CONSENT_HELPER_H_

#include "components/signin/core/browser/profile_management_switches.h"

class Profile;

// Returns the state of the "Unified Consent" feature for |profile|.
signin::UnifiedConsentFeatureState GetUnifiedConsentFeatureState(
    Profile* profile);

// Returns true if the unified consent feature state is kEnabledNoBump or
// kEnabledWithBump. Note that the bump may not be enabled, even if this returns
// true. To check if the bump is enabled, use IsUnifiedConsentBumpEnabled().
bool IsUnifiedConsentEnabled(Profile* profile);

// Returns true if the unified consent feature state is kEnabledWithBump.
bool IsUnifiedConsentBumpEnabled(Profile* profile);

#endif  // CHROME_BROWSER_SIGNIN_UNIFIED_CONSENT_HELPER_H_
