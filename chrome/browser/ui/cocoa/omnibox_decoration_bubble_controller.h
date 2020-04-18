// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_OMNIBOX_DECORATION_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_OMNIBOX_DECORATION_BUBBLE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/base_bubble_controller.h"

class LocationBarDecoration;

// Base bubble controller for bubbles that are anchored to an omnibox
// decoration. This controller updates the active state of the associated icon
// according to the state of the bubble.
@interface OmniboxDecorationBubbleController : BaseBubbleController

// Returns the omnibox icon the bubble is anchored to.
// Subclasses are expected to override this.
- (LocationBarDecoration*)decorationForBubble;

@end

#endif  // CHROME_BROWSER_UI_COCOA_OMNIBOX_DECORATION_BUBBLE_CONTROLLER_H_
