// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DEV_TOOLS_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_DEV_TOOLS_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/devtools/devtools_window.h"

@class FocusTracker;
@class DevToolsContainerView;
class Profile;

namespace content {
class WebContents;
}

// A class that handles updates of the devTools view within a browser window.
// It swaps in the relevant devTools contents for a given WebContents or removes
// the view, if there's no devTools contents to show.
@interface DevToolsController : NSObject {
 @private
  // A view hosting docked devTools contents.
  base::scoped_nsobject<DevToolsContainerView> devToolsContainerView_;

  base::scoped_nsobject<FocusTracker> focusTracker_;
}

- (id)init;

// This controller's view.
- (NSView*)view;

// Depending on |contents|'s state, decides whether the docked web inspector
// should be shown or hidden and adjusts inspected page position.
// Returns true iff layout has changed.
- (BOOL)updateDevToolsForWebContents:(content::WebContents*)contents
                         withProfile:(Profile*)profile;

@end

#endif  // CHROME_BROWSER_UI_COCOA_DEV_TOOLS_CONTROLLER_H_
