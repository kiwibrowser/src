// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PROFILE_CHOOSER_CONSTANTS_H_
#define CHROME_BROWSER_UI_PROFILE_CHOOSER_CONSTANTS_H_

#include "third_party/skia/include/core/SkColor.h"

namespace profiles {

static const SkColor kHoverColor = SkColorSetRGB(0xEA, 0xEA, 0xEA);

// Different views that can be displayed in the profile chooser bubble.
enum BubbleViewMode {
  // Shows the default avatar bubble.
  BUBBLE_VIEW_MODE_PROFILE_CHOOSER,
  // Shows a list of accounts for the active user.
  BUBBLE_VIEW_MODE_ACCOUNT_MANAGEMENT,
  // Shows a web view for primary sign in.
  BUBBLE_VIEW_MODE_GAIA_SIGNIN,
  // Shows a web view for adding secondary accounts.
  BUBBLE_VIEW_MODE_GAIA_ADD_ACCOUNT,
  // Shows a web view for reauthenticating an account.
  BUBBLE_VIEW_MODE_GAIA_REAUTH,
  // Shows a view for confirming account removal.
  BUBBLE_VIEW_MODE_ACCOUNT_REMOVAL,
};

};  // namespace profiles

#endif  // CHROME_BROWSER_UI_PROFILE_CHOOSER_CONSTANTS_H_
