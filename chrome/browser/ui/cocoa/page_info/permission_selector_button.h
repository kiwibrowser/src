// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PAGE_INFO_PERMISSION_SELECTOR_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_PAGE_INFO_PERMISSION_SELECTOR_BUTTON_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/page_info/permission_menu_model.h"
#include "components/content_settings/core/common/content_settings.h"

@class MenuControllerCocoa;

class Profile;

@interface PermissionSelectorButton : NSPopUpButton {
 @private
  std::unique_ptr<PermissionMenuModel> menuModel_;
  base::scoped_nsobject<MenuControllerCocoa> menuController_;
}

// Designated initializer.
- (id)initWithPermissionInfo:(const PageInfoUI::PermissionInfo&)permissionInfo
                      forURL:(const GURL&)url
                withCallback:(PermissionMenuModel::ChangeCallback)callback
                     profile:(Profile*)profile;

// Returns the largest possible size given all of the items in the menu.
- (CGFloat)maxTitleWidthForContentSettingsType:(ContentSettingsType)type
                            withDefaultSetting:(ContentSetting)defaultSetting
                                       profile:(Profile*)profile;

// Updates the title of the NSPopUpButton and resizes it to fit the new text.
- (void)setButtonTitle:(const PageInfoUI::PermissionInfo&)permissionInfo
               profile:(Profile*)profile;

@end

#endif  // CHROME_BROWSER_UI_COCOA_PAGE_INFO_PERMISSION_SELECTOR_BUTTON_H_
