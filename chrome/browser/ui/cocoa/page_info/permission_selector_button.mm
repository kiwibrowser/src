// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/page_info/permission_selector_button.h"

#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/cocoa/page_info/page_info_utils_cocoa.h"
#include "chrome/browser/ui/page_info/page_info_ui.h"
#import "ui/base/cocoa/menu_controller.h"

@implementation PermissionSelectorButton

- (id)initWithPermissionInfo:(const PageInfoUI::PermissionInfo&)permissionInfo
                      forURL:(const GURL&)url
                withCallback:(PermissionMenuModel::ChangeCallback)callback
                     profile:(Profile*)profile {
  if (self = [super initWithFrame:NSMakeRect(0, 0, 1, 1) pullsDown:NO]) {
    [self setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
    [self setBordered:NO];
    [[self cell] setControlSize:NSSmallControlSize];

    menuModel_.reset(
        new PermissionMenuModel(profile, url, permissionInfo, callback));

    menuController_.reset([[MenuControllerCocoa alloc]
                 initWithModel:menuModel_.get()
        useWithPopUpButtonCell:NO]);
    [self setMenu:[menuController_ menu]];
    [self selectItemWithTag:permissionInfo.setting];

    [self setButtonTitle:permissionInfo profile:profile];

    NSString* description = base::SysUTF16ToNSString(
        PageInfoUI::PermissionTypeToUIString(permissionInfo.type));
    [[self cell]
        accessibilitySetOverrideValue:description
                         forAttribute:NSAccessibilityDescriptionAttribute];
  }
  return self;
}

- (CGFloat)maxTitleWidthForContentSettingsType:(ContentSettingsType)type
                            withDefaultSetting:(ContentSetting)defaultSetting
                                       profile:(Profile*)profile {
  // Determine the largest possible size for this button.
  CGFloat maxTitleWidth = 0;
  for (NSMenuItem* item in [self itemArray]) {
    NSString* title =
        base::SysUTF16ToNSString(PageInfoUI::PermissionActionToUIString(
            profile, type, static_cast<ContentSetting>([item tag]),
            defaultSetting, content_settings::SETTING_SOURCE_USER));
    NSSize size = SizeForPageInfoButtonTitle(self, title);
    maxTitleWidth = std::max(maxTitleWidth, size.width);
  }
  return maxTitleWidth;
}

// Accessor function for testing only.
- (NSMenu*)permissionMenu {
  return [menuController_ menu];
}

- (void)setButtonTitle:(const PageInfoUI::PermissionInfo&)permissionInfo
               profile:(Profile*)profile {
  // Set the button title.
  base::scoped_nsobject<NSMenuItem> titleItem([[NSMenuItem alloc] init]);
  base::string16 buttonTitle = PageInfoUI::PermissionActionToUIString(
      profile, permissionInfo.type, permissionInfo.setting,
      permissionInfo.default_setting, permissionInfo.source);
  [titleItem setTitle:base::SysUTF16ToNSString(buttonTitle)];
  [[self cell] setUsesItemFromMenu:NO];
  [[self cell] setMenuItem:titleItem];
  // Although the frame is reset, below, this sizes the cell properly.
  [self sizeToFit];

  // Size the button to just fit the visible title - not all of its items.
  [self setFrameSize:SizeForPageInfoButtonTitle(self, [self title])];
}

@end
