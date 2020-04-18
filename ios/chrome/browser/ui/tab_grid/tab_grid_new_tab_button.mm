// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/tab_grid_new_tab_button.h"

#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabGridNewTabButton ()
@property(nonatomic, strong) UIImage* incognitoImage;
@property(nonatomic, strong) UIImage* regularImage;
@end

@implementation TabGridNewTabButton
// Public properties.
@synthesize page = _page;
@synthesize sizeClass = _sizeClass;
// Private properties.
@synthesize incognitoImage;
@synthesize regularImage;

+ (instancetype)buttonWithSizeClass:(TabGridNewTabButtonSizeClass)sizeClass {
  TabGridNewTabButton* button = [super buttonWithType:UIButtonTypeSystem];
  button.sizeClass = sizeClass;
  return button;
}

#pragma mark - Public

- (void)setPage:(TabGridPage)page {
  UIImage* renderedImage;
  switch (page) {
    case TabGridPageIncognitoTabs:
      self.enabled = YES;
      self.accessibilityLabel =
          l10n_util::GetNSString(IDS_IOS_TAB_GRID_CREATE_NEW_INCOGNITO_TAB);
      renderedImage = [self.incognitoImage
          imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
      break;
    case TabGridPageRegularTabs:
      self.enabled = YES;
      self.accessibilityLabel =
          l10n_util::GetNSString(IDS_IOS_TAB_GRID_CREATE_NEW_TAB);
      renderedImage = [self.regularImage
          imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
      break;
    case TabGridPageRemoteTabs:
      self.enabled = NO;
      // Accessibility label is same as the regular tabs button, except it will
      // say it is disabled.
      self.accessibilityLabel =
          l10n_util::GetNSString(IDS_IOS_TAB_GRID_CREATE_NEW_TAB);
      // The incognito new tab button image was made so that it can be a
      // template image. A template image becomes greyed out when disabled.
      renderedImage = [self.incognitoImage
          imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
      break;
  }
  [self setImage:renderedImage forState:UIControlStateNormal];
  _page = page;
}

- (void)setSizeClass:(TabGridNewTabButtonSizeClass)sizeClass {
  switch (sizeClass) {
    case TabGridNewTabButtonSizeClassSmall:
      self.incognitoImage =
          [UIImage imageNamed:@"new_tab_toolbar_button_incognito"];
      self.regularImage = [UIImage imageNamed:@"new_tab_toolbar_button"];
      break;
    case TabGridNewTabButtonSizeClassLarge:
      self.incognitoImage =
          [UIImage imageNamed:@"new_tab_floating_button_incognito"];
      self.regularImage = [UIImage imageNamed:@"new_tab_floating_button"];
      break;
  }
  _sizeClass = sizeClass;
}

@end
