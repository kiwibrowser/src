// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TOOLBAR_BACK_FORWARD_MENU_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_TOOLBAR_BACK_FORWARD_MENU_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/has_weak_browser_pointer.h"
#include "chrome/browser/ui/toolbar/back_forward_menu_model.h"

@class MenuButton;

typedef BackForwardMenuModel::ModelType BackForwardMenuType;
const BackForwardMenuType BACK_FORWARD_MENU_TYPE_BACK =
    BackForwardMenuModel::ModelType::kBackward;
const BackForwardMenuType BACK_FORWARD_MENU_TYPE_FORWARD =
    BackForwardMenuModel::ModelType::kForward;

// A class that manages the back/forward menu (and delayed-menu button, and
// model).
// Implement HasWeakBrowserPointer so we can clean up if the Browser is
// destroyed.
@interface BackForwardMenuController
    : NSObject<NSMenuDelegate, HasWeakBrowserPointer> {
 @private
  BackForwardMenuType type_;
  MenuButton* button_;  // Weak; comes from nib.
  std::unique_ptr<BackForwardMenuModel> model_;
  base::scoped_nsobject<NSMenu> backForwardMenu_;
}

// Type (back or forwards); can only be set on initialization.
@property(readonly, nonatomic) BackForwardMenuType type;

- (id)initWithBrowser:(Browser*)browser
            modelType:(BackForwardMenuType)type
               button:(MenuButton*)button;

@end  // @interface BackForwardMenuController

#endif  // CHROME_BROWSER_UI_COCOA_TOOLBAR_BACK_FORWARD_MENU_CONTROLLER_H_
