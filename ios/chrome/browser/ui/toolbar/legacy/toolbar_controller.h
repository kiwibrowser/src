// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_positioner.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_ui_element.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_constants.h"
#import "ios/chrome/browser/ui/toolbar/legacy/abstract_toolbar.h"
#import "ios/chrome/browser/ui/toolbar/legacy/legacy_toolbar_view.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller_constants.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_presentation_provider.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_presentation_state_provider.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@protocol OmniboxFocuser;
class ReadingListModel;
@protocol ToolbarCommands;

// Base class for a toolbar, containing the standard button set that is
// common across different types of toolbars and action handlers for those
// buttons (forwarding to the delegate). This is not intended to be used
// on its own, but to be subclassed by more specific toolbars that provide
// more buttons in the empty space.
@interface ToolbarController : UIViewController<AbstractToolbar,
                                                ActivityServicePositioner,
                                                FullscreenUIElement,
                                                ToolsMenuPresentationProvider>

// The top-level toolbar view.
@property(nonatomic, strong) LegacyToolbarView* view;
// The view for the toolbar shadow image.  This is a subview of |view| to
// allow clients to alter the visibility of the shadow without affecting other
// components of the toolbar.
@property(nonatomic, readonly, strong) UIImageView* shadowView;

// Returns the constraint controlling the height of the toolbar. If the
// constraint does not exist, creates it but does not activate it.
@property(nonatomic, readonly) NSLayoutConstraint* heightConstraint;

// The reading list model reflected by the toolbar.
@property(nonatomic, readwrite, assign) ReadingListModel* readingListModel;

// The command dispatcher this and any subordinate objects should use.
@property(nonatomic, readonly, weak)
    id<ApplicationCommands, BrowserCommands, OmniboxFocuser, ToolbarCommands>
        dispatcher;

// Designated initializer.
//   |style| determines how the toolbar draws itself.
//   |dispatcher| is is the dispatcher for calling methods handled in other
//     parts of the app.
- (instancetype)initWithStyle:(ToolbarControllerStyle)style
                   dispatcher:(id<ApplicationCommands,
                                  BrowserCommands,
                                  OmniboxFocuser,
                                  ToolbarCommands>)dispatcher
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;

// Height and Y offset to account for the status bar. Overridden by subclasses
// if the toolbar shouldn't extend through the status bar.
- (CGFloat)statusBarOffset;

// Shows/hides iPhone toolbar views for when the new tab page is displayed.
- (void)hideViewsForNewTabPage:(BOOL)hide;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_H_
