// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const int64_t kNonInitialImageAdditionDelayNanosec = 500000LL;

// Macros for creating CGRects of height H, origin (0,0), with the portrait
// width of phone/pad devices.
// clang-format off
#define IPHONE_FRAME(H) { { 0, 0 }, { kPortraitWidth[IPHONE_IDIOM], H } }
#define IPAD_FRAME(H)   { { 0, 0 }, { kPortraitWidth[IPAD_IDIOM],   H } }
// Makes a two-element C array of CGRects as described above, one for each
// device idiom.
#define FRAME_PAIR(H) { IPHONE_FRAME(H), IPAD_FRAME(H) }
// clang-format on

const CGRect kToolbarFrame[INTERFACE_IDIOM_COUNT] = FRAME_PAIR(56);
const CGRect kBackgroundViewFrame[INTERFACE_IDIOM_COUNT] = FRAME_PAIR(56);
const CGRect kShadowViewFrame[INTERFACE_IDIOM_COUNT] = FRAME_PAIR(2);
const CGRect kFullBleedShadowViewFrame = IPHONE_FRAME(10);

const CGFloat kStackButtonNormalColors[] = {
    85.0 / 255.0,   // ToolbarControllerStyleLightMode
    238.0 / 255.0,  // ToolbarControllerStyleDarkMode
    238.0 / 255.0,  // ToolbarControllerStyleIncognitoMode
};
const int kStackButtonHighlightedColors[] = {
    0x4285F4,  // ToolbarControllerStyleLightMode
    0x888a8c,  // ToolbarControllerStyleDarkMode
    0x888a8c,  // ToolbarControllerStyleIncognitoMode
};

// clang-format off
const LayoutRect kStackButtonFrame =
{kPortraitWidth[IPHONE_IDIOM], {230, 4}, {48, 48}};
const LayoutRect kShareMenuButtonFrame =
{kPortraitWidth[IPAD_IDIOM], {680, 4}, {46, 48}};
const LayoutRect kToolsMenuButtonFrame[INTERFACE_IDIOM_COUNT] = {
  {kPortraitWidth[IPHONE_IDIOM], {276, 4}, {44, 48}},
  {kPortraitWidth[IPAD_IDIOM], {723, 4}, {46, 48}}
};
// clang-format on

const LayoutOffset kButtonFadeOutXOffset = 10;
