// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/has_weak_browser_pointer.h"
#include "ui/gfx/geometry/size.h"

class Browser;
@class BrowserActionButton;
@class BrowserActionsContainerView;
@class MenuButton;
class ToolbarActionsBar;
@class ToolbarActionsBarBubbleMac;
class ToolbarActionsBarBubbleViewsPresenter;
class ToolbarActionsBarDelegate;

namespace content {
class WebContents;
}

// Sent when the visibility of the Browser Actions changes.
extern NSString* const kBrowserActionVisibilityChangedNotification;

// Handles state and provides an interface for controlling the Browser Actions
// container within the Toolbar.
@interface BrowserActionsController
    : NSObject<NSMenuDelegate, HasWeakBrowserPointer> {
 @private
  // Reference to the current browser. Weak.
  Browser* browser_;

  // The view from Toolbar.xib we'll be rendering our browser actions in. Weak.
  BrowserActionsContainerView* containerView_;

  // Array of toolbar action buttons in the correct order for them to be
  // displayed (includes both hidden and visible buttons).
  base::scoped_nsobject<NSMutableArray> buttons_;

  // The delegate for the ToolbarActionsBar.
  std::unique_ptr<ToolbarActionsBarDelegate> toolbarActionsBarBridge_;

  // The controlling ToolbarActionsBar.
  std::unique_ptr<ToolbarActionsBar> toolbarActionsBar_;

  // True if this is the overflow container for toolbar actions.
  BOOL isOverflow_;

  // The bubble that is actively showing, if any.
  ToolbarActionsBarBubbleMac* activeBubble_;

  // The index of the currently-focused view in the overflow menu, or -1 if
  // no view is focused.
  NSInteger focusedViewIndex_;

  // Bridge for showing the toolkit-views bubble on a Cocoa browser.
  std::unique_ptr<ToolbarActionsBarBubbleViewsPresenter> viewsBubblePresenter_;

  // True if a toolbar action button is being dragged.
  BOOL isDraggingSession_;
}

@property(nonatomic) CGFloat maxWidth;
@property(readonly, nonatomic) BrowserActionsContainerView* containerView;
@property(readonly, nonatomic) Browser* browser;
@property(readonly, nonatomic) BOOL isOverflow;
@property(readonly, nonatomic) ToolbarActionsBarBubbleMac* activeBubble;

// Initializes the controller given the current browser and container view that
// will hold the browser action buttons. If |mainController| is nil, the created
// BrowserActionsController will be the main controller; otherwise (if this is
// for the overflow menu), |mainController| should be controller of the main bar
// for the |browser|.
- (id)initWithBrowser:(Browser*)browser
        containerView:(BrowserActionsContainerView*)container
       mainController:(BrowserActionsController*)mainController;

// Update the display of all buttons.
- (void)update;

// Returns the current number of browser action buttons within the container,
// whether or not they are displayed.
- (NSUInteger)buttonCount;

// Returns the current number of browser action buttons displayed in the
// container.
- (NSUInteger)visibleButtonCount;

// Returns the preferred size for the container.
- (gfx::Size)preferredSize;

// Returns where the popup arrow should point to for the action with the given
// |id|. If passed an id with no corresponding button, returns NSZeroPoint.
- (NSPoint)popupPointForId:(const std::string&)id;

// Returns the currently-active web contents.
- (content::WebContents*)currentWebContents;

// Returns the BrowserActionButton in the main browser actions container (as
// opposed to the overflow) for the action of the given id.
- (BrowserActionButton*)mainButtonForId:(const std::string&)id;

// Returns the associated ToolbarActionsBar.
- (ToolbarActionsBar*)toolbarActionsBar;

// Sets whether or not the overflow container is focused in the app menu.
- (void)setFocusedInOverflow:(BOOL)focused;

// Returns the size for the provided |maxWidth| of the overflow menu.
- (gfx::Size)sizeForOverflowWidth:(int)maxWidth;

// Called when the window for the active bubble is closing, and sets the active
// bubble to nil.
- (void)bubbleWindowClosing:(NSNotification*)notification;

@end  // @interface BrowserActionsController

@interface BrowserActionsController(TestingAPI)
- (BrowserActionButton*)buttonWithIndex:(NSUInteger)index;
@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTROLLER_H_
