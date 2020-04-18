// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_CONSTANTS_H_

#import <Foundation/Foundation.h>

#import <CoreGraphics/CoreGraphics.h>
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"

// The time delay before non-initial button images are loaded.
extern const int64_t kNonInitialImageAdditionDelayNanosec;

// Toolbar frames shared with subclasses.
extern const CGRect kToolbarFrame[INTERFACE_IDIOM_COUNT];
// UI frames.  iPhone values followed by iPad values.
// Full-width frames that don't change for RTL languages.
extern const CGRect kBackgroundViewFrame[INTERFACE_IDIOM_COUNT];
extern const CGRect kShadowViewFrame[INTERFACE_IDIOM_COUNT];
// Full bleed shadow frame is iPhone-only
extern const CGRect kFullBleedShadowViewFrame;

// Color constants for the stack button text, normal and pressed states.  These
// arrays are indexed by ToolbarControllerStyle enum values.
extern const CGFloat kStackButtonNormalColors[];
extern const int kStackButtonHighlightedColors[];

// Frames that change for RTL.
extern const LayoutRect kStackButtonFrame;
extern const LayoutRect kShareMenuButtonFrame;
extern const LayoutRect kToolsMenuButtonFrame[INTERFACE_IDIOM_COUNT];

// Distance to shift buttons when fading out.
extern const LayoutOffset kButtonFadeOutXOffset;

enum ToolbarButtonUIState {
  ToolbarButtonUIStateNormal = 0,
  ToolbarButtonUIStatePressed,
  ToolbarButtonUIStateDisabled,
  NumberOfToolbarButtonUIStates,
};

// This enumerates the different buttons used by the toolbar and is used to map
// the resource IDs for the button's icons.  Subclasses with additional buttons
// should extend these values.  The first new enum should be set to
// |NumberOfToolbarButtonNames|.  Note that functions that use these values use
// an int rather than the |ToolbarButtonName| to accommodate additional values.
// Also, if this enum is extended by a subclass, the subclass must also override
// -imageIdForImageEnum:style:forState: to provide mapping from enum to resource
// ID for the various states.
enum ToolbarButtonName {
  ToolbarButtonNameStack = 0,
  ToolbarButtonNameShare,
  NumberOfToolbarButtonNames,
};

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_CONSTANTS_H_
