// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FIRST_RUN_STATIC_FILE_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_FIRST_RUN_STATIC_FILE_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

namespace ios {
class ChromeBrowserState;
}

// Status for tapped links.  This enum is used in UMA and entries should not be
// re-ordered or deleted.
enum MobileFreLinkTappedStatus {
  SUCCESS = 0,
  FAILED = 1,
  DID_NOT_COMPLETE = 2,
  NUM_MOBILE_FRE_LINK_TAPPED_STATUS
};

// View controller used to display a bundled file in a web view with a shadow
// below the navigation bar when the user scrolls.
@interface StaticFileViewController : UIViewController

// Initializes with the given URL to display and browser state. Neither
// |browserState| nor |URL| may be nil.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                                 URL:(NSURL*)URL;

// The status of the load.
@property(nonatomic, assign) MobileFreLinkTappedStatus loadStatus;

@end

#endif  // IOS_CHROME_BROWSER_UI_FIRST_RUN_STATIC_FILE_VIEW_CONTROLLER_H_
