// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"

#import "base/logging.h"
#include "ios/web/public/user_agent.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ToolsMenuConfiguration

@synthesize inTabSwitcher = _inTabSwitcher;
@synthesize noOpenedTabs = _noOpenedTabs;
@synthesize inIncognito = _inIncognito;
@synthesize showReadingListNewBadge = _showReadingListNewBadge;
@synthesize highlightNewIncognitoTabCell = _highlightNewIncognitoTabCell;
@synthesize userAgentType = _userAgentType;
@synthesize requestStartTime = _requestStartTime;
@synthesize engagementTracker = _engagementTracker;
@synthesize baseViewController = _baseViewController;
@synthesize displayView = _displayView;
@synthesize toolsMenuButton = _toolsMenuButton;
@synthesize readingListMenuNotifier = _readingListMenuNotifier;

- (instancetype)initWithDisplayView:(UIView*)displayView
                 baseViewController:(UIViewController*)baseViewController {
  if (self = [super init]) {
    _userAgentType = web::UserAgentType::NONE;
    _baseViewController = baseViewController;
    _displayView = displayView;
    _readingListMenuNotifier = nil;
    _engagementTracker = nullptr;
  }
  return self;
}

- (UIEdgeInsets)toolsButtonInsets {
  return self.toolsMenuButton ? [self.toolsMenuButton imageEdgeInsets]
                              : UIEdgeInsetsZero;
}

- (CGRect)sourceRect {
  // Set the origin for the tools popup to the horizontal center of the tools
  // menu button.
  return self.toolsMenuButton
             ? [self.displayView convertRect:self.toolsMenuButton.bounds
                                    fromView:self.toolsMenuButton]
             : CGRectZero;
}

@end
