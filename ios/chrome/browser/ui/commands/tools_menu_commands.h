// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_TOOLS_MENU_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_TOOLS_MENU_COMMANDS_H_

#import <Foundation/Foundation.h>

// Protocol that describes the commands that may trigger the presentation
// and dismissal of the Tools menu.
// TODO(crbug.com/800266): Remove this protocol once Phase 1 is enabled.
@protocol ToolsMenuCommands
// Display the tools menu.
- (void)showToolsMenu;

// Dismiss the tools menu.
- (void)dismissToolsMenu;
@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_TOOLS_MENU_COMMANDS_H_
