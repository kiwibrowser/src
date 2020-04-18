// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_VIEW_ID_UTIL_H_
#define CHROME_BROWSER_UI_COCOA_VIEW_ID_UTIL_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/ui/view_ids.h"
#include "ui/gfx/native_widget_types.h"

// ViewIDs are a system that indexes important views in the browser window by a
// ViewID identifier (integer). This is a useful compatibility for finding a
// view object in cross-platform tests. See BrowserFocusTest.* for an example
// of how ViewIDs are used.

// For views with fixed ViewIDs, we add a -viewID method to them to return their
// ViewIDs directly. But for views with changeable ViewIDs, as NSView itself
// doesn't provide a facility to store its ViewID, to avoid modifying each
// individual classes for adding ViewID support, we use an internal map to store
// ViewIDs of each view and provide some utility functions for NSView to
// set/unset the ViewID and lookup a view with a specified ViewID.

namespace view_id_util {

// Associates the given ViewID with the view. It shall be called upon the view's
// initialization.
void SetID(NSView* view, ViewID viewID);

// Removes the association between the view and its ViewID. It shall be called
// just before the view's destruction.
void UnsetID(NSView* view);

// Returns the view with a specific ViewID in a window, or nil if no view in the
// window has that ViewID.
NSView* GetView(NSWindow* window, ViewID viewID);

}  // namespace view_id_util


@interface NSView (ViewID)

// Returns the ViewID associated to the receiver. The default implementation
// looks up the view's ViewID in the internal view to ViewID map. A subclass may
// override this method to return its fixed ViewID.
- (ViewID)viewID;

// Returns the ancestor view with a specific ViewID, or nil if no ancestor
// view has that ViewID.
- (NSView*)ancestorWithViewID:(ViewID)viewID;

@end

#endif  // CHROME_BROWSER_UI_COCOA_VIEW_ID_UTIL_H_
