// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_SETTINGS_MAIN_PAGE_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_SETTINGS_MAIN_PAGE_COMMANDS_H_

// Command protocol for commands related to the Settings Main Page.
@protocol SettingsMainPageCommands
// Called when the Material Cell Catalog cell is tapped.
- (void)showMaterialCellCatalog;
@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_SETTINGS_MAIN_PAGE_COMMANDS_H_
