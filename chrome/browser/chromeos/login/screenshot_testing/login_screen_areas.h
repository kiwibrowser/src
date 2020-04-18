// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_LOGIN_SCREEN_AREAS_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_LOGIN_SCREEN_AREAS_H_

#include "third_party/skia/include/core/SkRect.h"

namespace chromeos {

namespace areas {

// Part of the screen with clock (when there are two digits in hours).
const SkIRect kClockArea = SkIRect::MakeLTRB(1260, 727, 1362 + 1, 758 + 1);

// First usedrpod, chosen as main, when we've got two users.
const SkIRect kFirstUserpod = SkIRect::MakeLTRB(493, 262, 652 + 1, 421 + 1);

// Second userpod when we've got two users.
const SkIRect kSecondUserpod = SkIRect::MakeLTRB(721, 271, 864 + 1, 415 + 1);

}  // namespace areas

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENSHOT_TESTING_LOGIN_SCREEN_AREAS_H_
