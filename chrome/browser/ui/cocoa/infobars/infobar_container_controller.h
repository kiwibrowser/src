// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTAINER_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTAINER_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/view_resizer.h"

@class BrowserWindowController;
@class InfoBarController;
class InfoBarCocoa;
class InfoBarContainerCocoa;

namespace content {
class WebContents;
}

// Controller for the infobar container view, which is the superview
// of all the infobar views.  This class owns zero or more
// InfoBarControllers, which manage the infobar views.  This class
// also receives tab strip model notifications and handles
// adding/removing infobars when needed.
@interface InfoBarContainerController : NSViewController {
 @private
  // Needed to send resize messages when infobars are added or removed.
  id<ViewResizer> resizeDelegate_;  // weak

  // The WebContents we are currently showing infobars for.
  content::WebContents* currentWebContents_;  // weak

  // Holds the InfoBarControllers currently owned by this container.
  base::scoped_nsobject<NSMutableArray> infobarControllers_;

  // The C++ instance that bridges to the cross platform code.
  std::unique_ptr<InfoBarContainerCocoa> containerCocoa_;

  // If YES then an infobar animation is in progress.
  BOOL isAnimating_;
}

- (id)initWithResizeDelegate:(id<ViewResizer>)resizeDelegate;

// Modifies this container to display infobars for the given |contents|.
- (void)changeWebContents:(content::WebContents*)contents;

// Stripped down version of TabStripModelObserverBridge:tabDetachedWithContents.
// Forwarded by BWC. Removes all infobars if |contents| is the current tab
// contents.
- (void)tabDetachedWithContents:(content::WebContents*)contents;

// Adds the given infobar.
- (void)addInfoBar:(InfoBarCocoa*)infobar
          position:(NSUInteger)position;

// Removes the given infobar.
- (void)removeInfoBar:(InfoBarCocoa*)infobar;

// Positions the infobar views in the container view and notifies
// |browser_controller_| that it needs to resize the container view.
- (void)positionInfoBarsAndRedraw:(BOOL)isAnimating;

// The height of all the info bars.
- (CGFloat)heightOfInfoBars;

@end

#endif  // CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTAINER_CONTROLLER_H_
