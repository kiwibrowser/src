// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_RESOURCE_MACROS_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_RESOURCE_MACROS_H_

#include "ios/chrome/grit/ios_theme_resources.h"

// These macros are used to populate the 3-dimensional array used to map from
// a button's name/style/state combination to resource ID pointing to the image
// to be used for the button.
// Each macro should "produce" a 2x3 entry for a button specified by |name|
// where style ∈ [light, dark] and state ∈ [normal, pressed, disabled].
// 0 is used when the resource for a given combination does not exist.

// clang-format off
#define TOOLBAR_IDR_THREE_STATE(name) \
  { { IDR_IOS_TOOLBAR_LIGHT_ ## name, \
      IDR_IOS_TOOLBAR_LIGHT_ ## name ## _PRESSED, \
      IDR_IOS_TOOLBAR_LIGHT_ ## name ## _DISABLED }, \
    { IDR_IOS_TOOLBAR_DARK_ ## name, \
      IDR_IOS_TOOLBAR_DARK_ ## name ## _PRESSED, \
      IDR_IOS_TOOLBAR_DARK_ ## name ## _DISABLED } }

#define TOOLBAR_IDR_TWO_STATE(name) \
  { { IDR_IOS_TOOLBAR_LIGHT_ ## name, \
      IDR_IOS_TOOLBAR_LIGHT_ ## name ## _PRESSED, 0 }, \
    { IDR_IOS_TOOLBAR_DARK_ ## name, \
      IDR_IOS_TOOLBAR_DARK_ ## name ## _PRESSED, 0 } }

#define TOOLBAR_IDR_ONE_STATE(name) \
  { { IDR_IOS_TOOLBAR_LIGHT_ ## name, 0, 0 }, \
    { IDR_IOS_TOOLBAR_LIGHT_ ## name, 0, 0 } }

#define TOOLBAR_IDR_LIGHT_ONLY_THREE_STATE(name) \
  { { IDR_IOS_TOOLBAR_LIGHT_ ## name, \
      IDR_IOS_TOOLBAR_LIGHT_ ## name ## _PRESSED, \
      IDR_IOS_TOOLBAR_LIGHT_ ## name ## _DISABLED }, \
      { 0, 0, 0 } }

#define TOOLBAR_IDR_LIGHT_ONLY_TWO_STATE(name) \
  { { IDR_IOS_TOOLBAR_LIGHT_ ## name, \
      IDR_IOS_TOOLBAR_LIGHT_ ## name ## _PRESSED, 0 }, \
      { 0, 0, 0 } }
// clang-format on

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_RESOURCE_MACROS_H_
