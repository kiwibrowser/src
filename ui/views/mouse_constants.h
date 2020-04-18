// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MOUSE_CONSTANTS_H_
#define UI_VIEWS_MOUSE_CONSTANTS_H_


namespace views {

// The amount of time the mouse should be down before a mouse release is
// considered intentional. This is to prevent spurious mouse releases from
// activating controls, especially when some UI element is revealed under the
// source of the activation (ex. menus showing underneath menu buttons).
const int kMinimumMsPressedToActivate = 200;

// The amount of time, in milliseconds, between clicks until they're
// considered intentionally different.
const int kMinimumMsBetweenButtonClicks = 100;

}  // namespace views

#endif  // UI_VIEWS_MOUSE_CONSTANTS_H_
