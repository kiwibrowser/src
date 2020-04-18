// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/hover_image_button.h"

// A subclass of HoverImageButton that sends a target-action on right click and
// has an active state.
@interface AvatarButton : HoverImageButton {
 @private
  SEL rightAction_;
}

// YES if the button is active. The button should only be active when the
// avatar bubble is shown.
@property(nonatomic, assign) BOOL isActive;

// Sets the action that will be called when this button is right clicked.
- (void)setRightAction:(SEL)selector;

@end

#endif //CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BUTTON_H_
