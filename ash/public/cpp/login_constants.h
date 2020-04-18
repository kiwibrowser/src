// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_LOGIN_CONSTANTS_H_
#define ASH_PUBLIC_CPP_LOGIN_CONSTANTS_H_

#include "third_party/skia/include/core/SkColor.h"

// This file exists in //ash/public because the values are shared between webui
// login code in chrome and views-based login code in ash. If the webui login
// code is ever removed this file can move to //ash/login.

namespace ash {
namespace login_constants {

// The default base color of the login/lock screen when the dark muted color
// extracted from wallpaper is invalid.
constexpr SkColor kDefaultBaseColor = SK_ColorBLACK;

// The alpha value for the login/lock screen background.
constexpr int kTranslucentAlpha = 153;

// The alpha value for the scrollable container on the account picker.
constexpr int kScrollTranslucentAlpha = 76;

// The alpha value used to darken the login/lock screen.
constexpr int kTranslucentColorDarkenAlpha = 128;

// The blur sigma for login/lock screen.
constexpr float kBlurSigma = 30.0f;

// The sigma to clear the blur from login/lock screen.
constexpr float kClearBlurSigma = 0.0f;

// How long should animations that change the layout of the user run for? For
// example, this includes the user switch animation as well as the PIN keyboard
// show/hide animation.
constexpr int kChangeUserAnimationDurationMs = 300;

// Color of enabled buttons.
constexpr SkColor kButtonEnabledColor = SK_ColorWHITE;

// An alpha value for disabled buttons.
// In specs this is listed as 34% = 0x57 / 0xFF.
constexpr SkAlpha kButtonDisabledAlpha = 0x57;

}  // namespace login_constants
}  // namespace ash

#endif  // ASH_PUBLIC_CPP_LOGIN_CONSTANTS_H_
