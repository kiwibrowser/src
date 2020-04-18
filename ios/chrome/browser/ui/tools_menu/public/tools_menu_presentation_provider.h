// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_PRESENTATION_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_PRESENTATION_PROVIDER_H_

#import <Foundation/Foundation.h>

@class ToolsMenuCoordinator, UIButton;

// A protocol which allows details of the presentation of the tools menu to be
// configured.
@protocol ToolsMenuPresentationProvider
// Returns the button being used to present and dismiss the tools menu,
// if applicable.
- (UIButton*)presentingButtonForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_PRESENTATION_PROVIDER_H_
