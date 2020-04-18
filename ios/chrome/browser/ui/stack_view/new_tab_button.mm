// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/stack_view/new_tab_button.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/commands/open_new_tab_command.h"
#import "ios/chrome/browser/ui/image_util/image_util.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Amount by which to inset the button's content.
const CGFloat kContentInset = 6.0;

// Duration of transition animation.
const NSTimeInterval kNewTabButtonTransitionDuration =
    ios::material::kDuration1;
}

@implementation NewTabButton

@synthesize incognito = _incognito;

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    self.incognito = NO;

    [self
        setContentEdgeInsets:UIEdgeInsetsMakeDirected(0, kContentInset, 0, 0)];
    [self setContentHorizontalAlignment:
              UseRTLLayout() ? UIControlContentHorizontalAlignmentRight
                             : UIControlContentHorizontalAlignmentLeft];
  }
  return self;
}

- (void)setIncognito:(BOOL)incognito {
  NSString* normalImageName = @"toolbar_dark_newtab";
  NSString* activeImageName = @"toolbar_dark_newtab_active";
  if (incognito) {
    SetA11yLabelAndUiAutomationName(self, IDS_IOS_TOOLS_MENU_NEW_INCOGNITO_TAB,
                                    @"New Incognito Tab");
    normalImageName = @"toolbar_dark_newtab_incognito";
    activeImageName = @"toolbar_dark_newtab_incognito_active";
  } else {
    SetA11yLabelAndUiAutomationName(self, IDS_IOS_TOOLS_MENU_NEW_TAB,
                                    @"New Tab");
  }
  [self setImage:[UIImage imageNamed:normalImageName]
        forState:UIControlStateNormal];
  [self setImage:[UIImage imageNamed:activeImageName]
        forState:UIControlStateHighlighted];
  _incognito = incognito;
}

- (void)setIncognito:(BOOL)incognito animated:(BOOL)animated {
  if (self.isIncognito == incognito)
    return;

  if (animated) {
    [UIView transitionWithView:self
                      duration:kNewTabButtonTransitionDuration
                       options:UIViewAnimationOptionTransitionCrossDissolve
                    animations:^{
                      self.incognito = incognito;
                    }
                    completion:nil];
  } else {
    self.incognito = incognito;
  }
}

@end
