// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_LAYOUT_GUIDE_NAMES_H_
#define IOS_CHROME_BROWSER_UI_UTIL_LAYOUT_GUIDE_NAMES_H_

#import <UIKit/UIKit.h>

typedef NSString GuideName;

// The list of well-known UILayoutGuides.  When adding a new guide to the app,
// create a constant for it below.

// A guide that is constrained to match the frame of the secondary toolbar.
extern GuideName* const kSecondaryToolbar;
// A guide that is constrained to match the frame the secondary toolbar would
// have if fullscreen was disabled.
extern GuideName* const kSecondaryToolbarNoFullscreen;
// A guide that is constrained to match the frame of the omnibox.
extern GuideName* const kOmniboxGuide;
// A guide that is constrained to match the frame of the back button's image.
extern GuideName* const kBackButtonGuide;
// A guide that is constrained to match the frame of the forward button's image.
extern GuideName* const kForwardButtonGuide;
// A guide that is constrained to match the frame of the Search button.
extern GuideName* const kSearchButtonGuide;
// A guide that is constrained to match the frame of the TabSwitcher button's
// image.
extern GuideName* const kTabSwitcherGuide;
// A guide that is constrained to match the frame of the ToolsMenu button.
extern GuideName* const kToolsMenuGuide;
// A guide that is constrained to match the frame of the last-tapped voice
// search button.
extern GuideName* const kVoiceSearchButtonGuide;

#endif  // IOS_CHROME_BROWSER_UI_UTIL_LAYOUT_GUIDE_NAMES_H_
