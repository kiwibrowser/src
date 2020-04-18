// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_manager_util_linux.h"

#if !defined(OS_CHROMEOS)

#include "base/strings/stringprintf.h"

const char kLibsecretAndGnomeAppString[] = "chrome";

// Generates a profile-specific app string based on profile_id_.
std::string GetProfileSpecificAppString(LocalProfileId id) {
  // Originally, the application string was always just "chrome" and used only
  // so that we had *something* to search for since GNOME Keyring won't search
  // for nothing. Now we use it to distinguish passwords for different profiles.
  return base::StringPrintf("%s-%d", kLibsecretAndGnomeAppString, id);
}

#endif  // !OS_CHROMEOS
