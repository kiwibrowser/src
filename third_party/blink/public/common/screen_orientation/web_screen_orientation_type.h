// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_SCREEN_ORIENTATION_WEB_SCREEN_ORIENTATION_TYPE_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_SCREEN_ORIENTATION_WEB_SCREEN_ORIENTATION_TYPE_H_

namespace blink {

enum WebScreenOrientationType {
  kWebScreenOrientationUndefined = 0,
  kWebScreenOrientationPortraitPrimary,
  kWebScreenOrientationPortraitSecondary,
  kWebScreenOrientationLandscapePrimary,
  kWebScreenOrientationLandscapeSecondary,

  WebScreenOrientationTypeLast = kWebScreenOrientationLandscapeSecondary
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_SCREEN_ORIENTATION_WEB_SCREEN_ORIENTATION_TYPE_H_
