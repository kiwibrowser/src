// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/google/core/browser/google_pref_names.h"

namespace prefs {

// String containing the last known Google URL.  We re-detect this on startup in
// most cases, and use it to send traffic to the correct Google host or with the
// correct Google domain/country code for whatever location the user is in.
const char kLastKnownGoogleURL[] = "browser.last_known_google_url";

// String containing the last prompted Google URL.
const char kLastPromptedGoogleURL[] = "browser.last_prompted_google_url";

}  // namespace prefs
