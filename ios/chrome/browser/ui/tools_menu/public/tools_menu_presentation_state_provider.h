// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_PRESENTATION_STATE_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_PRESENTATION_STATE_PROVIDER_H_

#import <Foundation/Foundation.h>

// A protocol which allows objects to be able to request information about
// the presentation state of the tools UI without having to have full
// knowledge of the ToolsMenuCoordinator class.
@protocol ToolsMenuPresentationStateProvider
// Returns whether the tools menu on screen and being presented to the user.
- (BOOL)isShowingToolsMenu;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_PRESENTATION_STATE_PROVIDER_H_
