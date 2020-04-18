// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BUBBLE_ANCHOR_HELPER_H_
#define CHROME_BROWSER_UI_COCOA_BUBBLE_ANCHOR_HELPER_H_

#import <Foundation/Foundation.h>

class Browser;

// Returns true if |browser| has a visible location bar.
bool HasVisibleLocationBarForBrowser(Browser* browser);

// Returns a point screen coordinates at the bottom left of the location bar
// when location bar is present and a point near the left edge of the screen
// otherwise in order to not obscure the fullscreen request bubble.
NSPoint GetPageInfoAnchorPointForBrowser(Browser* browser);

// TODO(tapted): Remove this overload. It's needed for the unit tests for Cocoa
// permission bubbles to force an anchoring type, which can be removed once
// http://crbug.com/740827 is fixed.
NSPoint GetPageInfoAnchorPointForBrowser(Browser* browser,
                                         bool has_location_bar);

#endif  // CHROME_BROWSER_UI_COCOA_BUBBLE_ANCHOR_HELPER_H_
