// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SWITCHES_CHROME_SWITCHES_H_
#define SWITCHES_CHROME_SWITCHES_H_

// This file defines switches that are used both by Chrome and login_manager.

namespace chromeos {
namespace switches {

// Sentinel switches for policy injected flags.
const char kPolicySwitchesBegin[] = "policy-switches-begin";
const char kPolicySwitchesEnd[] = "policy-switches-end";

// Flag passed to the browser if the system is running in dev-mode.
const char kSystemInDevMode[] = "system-developer-mode";

}  // namespace switches
}  // namespace chromeos

#endif  // SWITCHES_CHROME_SWITCHES_H_
