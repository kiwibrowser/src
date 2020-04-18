// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_ICON_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_ICON_CONTROLLER_H_

#import <AppKit/AppKit.h>

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_base_controller.h"

class Browser;

// This view controller manages the image that sits in the top of the window
// frame in Incognito, the spy dude. For regular and guest profiles,
// AvatarButtonController is used instead.
@interface AvatarIconController : AvatarBaseController {
}

// Designated initializer.
- (id)initWithBrowser:(Browser*)browser;

@end

#endif  // CHROME_BROWSER_UI_COCOA_PROFILES_AVATAR_ICON_CONTROLLER_H_
