// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PROFILES_PROFILE_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_PROFILES_PROFILE_UTIL_H_

class Profile;

namespace chromeos {

// Checks if the current log-in state and the profile is GAIA-authenticated and
// thus has access to Google services. This excludes, for example, public
// accounts, supervised users, guest or incognito profiles.
bool IsProfileAssociatedWithGaiaAccount(Profile* profile);

} // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PROFILES_PROFILE_UTIL_H_
