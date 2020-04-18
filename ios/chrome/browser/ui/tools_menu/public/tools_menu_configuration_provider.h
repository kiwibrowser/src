// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_CONFIGURATION_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_CONFIGURATION_PROVIDER_H_

#import <Foundation/Foundation.h>

@class ToolsMenuCoordinator, ToolsMenuConfiguration;

// A protocol that describes a set of methods which may configure a
// ToolsMenuCoordinator. Most of the configuration of the coordinator may be
// achieved through the required ToolsMenuConfiguration object, but some
// optional minor elements such as bookmark highlights are also independently
// configurable.
// TODO(crbug.com/800266): Remove this protocol.
@protocol ToolsMenuConfigurationProvider<NSObject>
// Returns a ToolsMenuConfiguration object describing the desired configuration
// of the tools menu.
- (ToolsMenuConfiguration*)menuConfigurationForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
@optional
// If implemented, may influence how the tools menu shows the bookmark
// indicator in the tools UI.
- (BOOL)shouldHighlightBookmarkButtonForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented, may influence the presence of the find bar in the tools
// UI.
- (BOOL)shouldShowFindBarForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented,may influence the presence of the share menu in the tools
// UI.
- (BOOL)shouldShowShareMenuForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented, may influence how the tools menu shows page-reload related
// UI.
- (BOOL)isTabLoadingForToolsMenuCoordinator:(ToolsMenuCoordinator*)coordinator;
// If implemented, allows the delegate to be informed of an imminent
// presentation of the tools menu. The delegate may choose to dismiss other
// presented UI or otherwise configure itself for the menu presentation.
- (void)prepareForToolsMenuPresentationByCoordinator:
    (ToolsMenuCoordinator*)coordinator;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_CONFIGURATION_PROVIDER_H_
