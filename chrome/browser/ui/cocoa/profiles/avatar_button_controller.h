// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BUTTON_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BUTTON_CONTROLLER_H_

#import <AppKit/AppKit.h>

#import "chrome/browser/ui/cocoa/profiles/avatar_base_controller.h"

class Browser;

// This view controller manages the button that sits in the top of the
// window frame when using multi-profiles, and shows the current profile's
// name. Clicking the button will open the profile menu.
@interface AvatarButtonController : AvatarBaseController {
 @private
  BOOL isThemedWindow_;
  // Whether the signed in profile has an authentication error. Used to
  // display an error icon next to the button text.
  BOOL hasError_;

  // The window associated with the avatar button. Weak.
  NSWindow* window_;
}
// Designated initializer. The parameter |window| is required because
// browser->window() might be null when the initalizer is called.
- (id)initWithBrowser:(Browser*)browser window:(NSWindow*)window;

// Returns YES if the browser window's frame color is dark.
- (BOOL)isFrameColorDark;

// Overridden so that we can change the active state of the avatar button.
- (void)showAvatarBubbleAnchoredAt:(NSView*)anchor
                          withMode:(BrowserWindow::AvatarBubbleMode)mode
                   withServiceType:(signin::GAIAServiceType)serviceType
                   fromAccessPoint:(signin_metrics::AccessPoint)accessPoint;

@end

#endif  // CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_BUTTON_CONTROLLER_H_
