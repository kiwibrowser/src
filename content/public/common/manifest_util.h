// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_MANIFEST_UTIL_H_
#define CONTENT_PUBLIC_COMMON_MANIFEST_UTIL_H_

#include <string>

#include "content/common/content_export.h"
#include "third_party/blink/public/common/manifest/web_display_mode.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_lock_type.h"

namespace content {

// Converts a blink::WebDisplayMode to a string. Returns one of
// https://www.w3.org/TR/appmanifest/#dfn-display-modes-values. Return values
// are lowercase. Returns an empty string for blink::WebDisplayModeUndefined.
CONTENT_EXPORT std::string WebDisplayModeToString(
    blink::WebDisplayMode display);

// Returns the blink::WebDisplayMode which matches |display|.
// |display| should be one of
// https://www.w3.org/TR/appmanifest/#dfn-display-modes-values. |display| is
// case insensitive. Returns blink::WebDisplayModeUndefined if there is no
// match.
CONTENT_EXPORT blink::WebDisplayMode WebDisplayModeFromString(
    const std::string& display);

// Converts a blink::WebScreenOrientationLockType to a string. Returns one of
// https://www.w3.org/TR/screen-orientation/#orientationlocktype-enum. Return
// values are lowercase. Returns an empty string for
// blink::WebScreenOrientationLockDefault.
CONTENT_EXPORT std::string WebScreenOrientationLockTypeToString(
    blink::WebScreenOrientationLockType);

// Returns the blink::WebScreenOrientationLockType which matches
// |orientation|. |orientation| should be one of
// https://www.w3.org/TR/screen-orientation/#orientationlocktype-enum.
// |orientation| is case insensitive. Returns
// blink::WebScreenOrientationLockDefault if there is no match.
CONTENT_EXPORT blink::WebScreenOrientationLockType
WebScreenOrientationLockTypeFromString(const std::string& orientation);

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_MANIFEST_UTIL_H_
