// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"
#include "base/strings/string16.h"
#include "ios/chrome/browser/ui/contextual_search/contextual_search_panel_state.h"

namespace ios {
class ChromeBrowserState;
}

namespace web {
class WebState;
}

class GURL;
@class Tab;
@class TouchToSearchPermissionsMediator;
struct WebStateOpener;

namespace ContextualSearch {
// Possible reason for panel state changes.
enum StateChangeReason {
  UNKNOWN,
  RESET,
  BACK_PRESS,  // This is Android-only; we should never use this value.
  TEXT_SELECT_TAP,
  TEXT_SELECT_LONG_PRESS,
  INVALID_SELECTION,
  BASE_PAGE_TAP,
  BASE_PAGE_SCROLL,
  SEARCH_BAR_TAP,
  SERP_NAVIGATION,
  TAB_PROMOTION,
  CLICK,
  SWIPE,  // Moved by going to the nearest state when a drag ends.
  FLING,  // Moved by interpolating a drag with velocity.
  OPTIN,
  OPTOUT
};
}  // namespace ContextualSearch

// Controller for handling contextual search gestures.
@interface ContextualSearchController : NSObject

// YES if contextual search is enabled on the current WebState.
@property(readonly) BOOL enabled;

// The current web state being watched for contextual search events.
// Only tests should set this directly; for general use, use -setTab:, which
// will set this property with the tab's webState.
@property(nonatomic, assign) web::WebState* webState;

// Designated initializer.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState;

// Set the Tab to be used as the opener for the search results tab. |opener|'s
// lifetime should be greater than the receiver's. |opener| can be nil.
- (void)setTab:(Tab*)tab;

// Enable or disable contextual search for the current WebState. If
// |enabled| is YES, contextual search may still not be enabled for a number of
// reasons; the -enabled property will indicate the current status.
// This method functions asynchronously.
- (void)enableContextualSearch:(BOOL)enabled;

// Reset the pane to be offscreen when tabs change.
- (void)movePanelOffscreen;

// Destroy the receiver.
// Any following call is not supported.
- (void)close;

@end

// Testing category that allows a permissions class to be injected.
@interface ContextualSearchController (Testing)

- (void)setPermissions:(TouchToSearchPermissionsMediator*)permissions;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_CONTEXTUAL_SEARCH_CONTROLLER_H_
