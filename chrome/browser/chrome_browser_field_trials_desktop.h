// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_DESKTOP_H_
#define CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_DESKTOP_H_

namespace chrome {

// Sets up common desktop-only field trials.
// Add an invocation of your field trial init function to this method, or to
// SetupFieldTrials in chrome_browser_field_trials.cc if it is for all
// platforms.
void SetupDesktopFieldTrials();

}  // namespace chrome

#endif  // CHROME_BROWSER_CHROME_BROWSER_FIELD_TRIALS_DESKTOP_H_
