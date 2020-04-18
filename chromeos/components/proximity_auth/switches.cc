// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/switches.h"

namespace proximity_auth {
namespace switches {

// Enables forcing the user to reauth with their password after X hours (e.g.
// 20) without password entry.
const char kEnableForcePasswordReauth[] = "force-password-reauth";

// Force easy unlock app loading in test.
// TODO(xiyuan): Remove this when app could be bundled with Chrome.
const char kForceLoadEasyUnlockAppInTests[] =
    "force-load-easy-unlock-app-in-tests";

}  // namespace switches
}  // namespace proximity_auth
