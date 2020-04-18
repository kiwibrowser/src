// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_session_cell_data.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation TabSwitcherSessionCellData

@synthesize type = _type;
@synthesize title = _title;
@synthesize image = _image;

+ (instancetype)incognitoSessionCellData {
  static TabSwitcherSessionCellData* incognitoSessionCellData = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    incognitoSessionCellData =
        [[self alloc] initWithSessionCellType:kIncognitoSessionCell];
  });
  return incognitoSessionCellData;
}

+ (instancetype)openTabSessionCellData {
  static TabSwitcherSessionCellData* openTabSessionCellData = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    openTabSessionCellData =
        [[self alloc] initWithSessionCellType:kOpenTabSessionCell];
  });
  return openTabSessionCellData;
}

+ (instancetype)otherDevicesSessionCellData {
  static TabSwitcherSessionCellData* otherDevicesSessionCellData = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    otherDevicesSessionCellData =
        [[self alloc] initWithSessionCellType:kGenericRemoteSessionCell];
  });
  return otherDevicesSessionCellData;
}

- (instancetype)initWithSessionCellType:(TabSwitcherSessionCellType)type {
  self = [super init];
  if (self) {
    _type = type;
    [self loadDefaultsForType];
  }
  return self;
}

#pragma mark - Private

- (void)loadDefaultsForType {
  NSString* imageName = nil;
  int messageId = 0;
  switch (self.type) {
    case kIncognitoSessionCell:
      imageName = @"tabswitcher_incognito";
      messageId = IDS_IOS_TAB_SWITCHER_HEADER_INCOGNITO_TABS;
      break;
    case kOpenTabSessionCell:
      imageName = @"tabswitcher_open_tabs";
      messageId = IDS_IOS_TAB_SWITCHER_HEADER_NON_INCOGNITO_TABS;
      break;
    case kGenericRemoteSessionCell:
      imageName = @"tabswitcher_other_devices";
      messageId = IDS_IOS_TAB_SWITCHER_HEADER_OTHER_DEVICES_TABS;
      break;
    case kPhoneRemoteSessionCell:
      imageName = @"ntp_opentabs_phone";
      messageId = IDS_IOS_TAB_SWITCHER_HEADER_OTHER_DEVICES_TABS;
      break;
    case kTabletRemoteSessionCell:
      imageName = @"ntp_opentabs_tablet";
      messageId = IDS_IOS_TAB_SWITCHER_HEADER_OTHER_DEVICES_TABS;
      break;
    case kLaptopRemoteSessionCell:
      imageName = @"ntp_opentabs_laptop";
      messageId = IDS_IOS_TAB_SWITCHER_HEADER_OTHER_DEVICES_TABS;
      break;
  }
  [self setTitle:l10n_util::GetNSString(messageId)];
  UIImage* image = [UIImage imageNamed:imageName];
  image = [image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  [self setImage:image];
}

@end
