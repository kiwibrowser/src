// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_TABS_TAB_PRIVATE_H_
#define IOS_CHROME_BROWSER_TABS_TAB_PRIVATE_H_

#include "ios/net/request_tracker.h"
#import "ios/web/public/web_state/ui/crw_web_delegate.h"

namespace web {
class NavigationItem;
class NavigationManagerImpl;
class WebStateImpl;
}

// Exposed private methods for testing purpose.
@interface Tab ()<CRWWebDelegate>

- (OpenInController*)openInController;

@end

@interface Tab (TestingSupport)

// Returns the Tab owning TabModel.
- (TabModel*)parentTabModel;

// Variant of -navigationManager that returns the NavigationManager as a
// NavigationManagerImpl. This should only be used by tests and will be
// removed when Tab can wrap TestWebState (see issue crbug.com/620465 for
// progress).
- (web::NavigationManagerImpl*)navigationManagerImpl;

// The CRWWebController from the Tab's WebState. This should only be used
// by tests and will be removed when Tab can wrap TestWebState (see issue
// crbug.com/620465 for progress).
@property(nonatomic, readonly) CRWWebController* webController;

@end

#endif  // IOS_CHROME_BROWSER_TABS_TAB_PRIVATE_H_
