// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox_decoration_bubble_controller.h"

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"

// Base bubble controller class.
@implementation OmniboxDecorationBubbleController

- (void)showWindow:(id)sender {
  LocationBarDecoration* decoration = [self decorationForBubble];
  if (decoration)
    decoration->SetActive(true);

  [super showWindow:sender];
}

- (void)close {
  LocationBarDecoration* decoration = [self decorationForBubble];
  if (decoration)
    decoration->SetActive(false);

  [super close];
}

- (LocationBarDecoration*)decorationForBubble {
  NOTREACHED();
  return nullptr;
}

@end
