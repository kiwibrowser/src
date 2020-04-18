// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_TAB_CONTENTS_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_TAB_CONTENTS_CONTROLLER_H_

#include <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"

class FullscreenObserver;
@class WebTextfieldTouchBarController;

namespace content {
class WebContents;
}

// A class that controls the WebContents view. It internally creates a container
// view (the NSView accessed by calling |-view|) which manages the layout and
// display of the WebContents view.
//
// Client code that inserts [controller view] into the view hierarchy needs to
// call -ensureContentsVisibleInSuperview:(NSView*)superview to match the
// container to the [superview bounds] and avoid multiple resize messages being
// sent to the renderer, which triggers redundant and costly layouts.
//
// AutoEmbedFullscreen mode: When enabled, TabContentsController will observe
// for WebContents fullscreen changes and automatically swap the normal
// WebContents view with the fullscreen view (if different). In addition, if a
// WebContents is being screen-captured, the view will be centered within the
// container view, sized to the aspect ratio of the capture video resolution,
// and scaling will be avoided whenever possible.
@interface TabContentsController : NSViewController {
 @private
   content::WebContents* contents_;  // weak
   // When |fullscreenObserver_| is not-NULL, TabContentsController monitors for
   // and auto-embeds fullscreen widgets as a subview.
   std::unique_ptr<FullscreenObserver> fullscreenObserver_;
   // Set to true while TabContentsController is embedding a fullscreen widget
   // view as a subview instead of the normal WebContentsView render view.
   // Note: This will be false in the case of non-Flash fullscreen.
   BOOL isEmbeddingFullscreenWidget_;

   // Set to true if the window is a popup.
   BOOL isPopup_;

   // Reference to the FullscreenPlaceholderView displayed in the main window
   // for the tab when our WebContentsView is in the SeparateFullscreenWindow.
   NSView* fullscreenPlaceholderView_;
   // Reference to the fullscreen window created to display the WebContents
   // view separately.
   NSWindow* separateFullscreenWindow_;

   base::scoped_nsobject<WebTextfieldTouchBarController> touchBarController_;
}
@property(readonly, nonatomic) content::WebContents* webContents;

// This flag is set to true when we don't want the fullscreen widget to
// resize. This is done so that we can avoid resizing the fullscreen widget
// to intermediate sizes during the fullscreen transition.
// As a result, we would prevent janky movements during the transition and
// Pepper Fullscreen from blowing up.
@property(assign, nonatomic) BOOL blockFullscreenResize;

// Create the contents of a tab represented by |contents|.
- (id)initWithContents:(content::WebContents*)contents isPopup:(BOOL)popup;

// Call to insert the container view into the view hierarchy, sizing it to match
// |superview|. Then, this method will select either the WebContents view or
// the fullscreen view and swap it into the container for display.
- (void)ensureContentsVisibleInSuperview:(NSView*)superview;

// Called after we enter fullscreen to ensure that the fullscreen widget will
// have the right frame.
- (void)updateFullscreenWidgetFrame;

// Call to change the underlying web contents object. View is not changed,
// call |-ensureContentsVisible| to display the |newContents|'s render widget
// host view.
- (void)changeWebContents:(content::WebContents*)newContents;

// Called when the tab contents is the currently selected tab and is about to be
// removed from the view hierarchy.
- (void)willBecomeUnselectedTab;

// Called when the tab contents is about to be put into the view hierarchy as
// the selected tab. Handles things such as ensuring the toolbar is correctly
// enabled.
- (void)willBecomeSelectedTab;

// Called when the tab contents is updated in some non-descript way (the
// notification from the model isn't specific). |updatedContents| could reflect
// an entirely new tab contents object.
- (void)tabDidChange:(content::WebContents*)updatedContents;

// Called to switch the container's subview to the WebContents-owned fullscreen
// widget or back to WebContentsView's widget.
- (void)toggleFullscreenWidget:(BOOL)enterFullscreen;

- (WebTextfieldTouchBarController*)webTextfieldTouchBarController;

@end

#endif  // CHROME_BROWSER_UI_COCOA_TAB_CONTENTS_TAB_CONTENTS_CONTROLLER_H_
