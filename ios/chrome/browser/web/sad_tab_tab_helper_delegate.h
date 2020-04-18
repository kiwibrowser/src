// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_SAD_TAB_TAB_HELPER_DELEGATE_H_
#define IOS_CHROME_BROWSER_WEB_SAD_TAB_TAB_HELPER_DELEGATE_H_

#import <Foundation/Foundation.h>

class SadTabTabHelper;

namespace web {
class WebState;
}

// Delegate for SadTabTabHelper.
@protocol SadTabTabHelperDelegate<NSObject>

// Asks the delegate to present a SadTabView.
- (void)sadTabTabHelper:(SadTabTabHelper*)tabHelper
    presentSadTabForWebState:(web::WebState*)webState
             repeatedFailure:(BOOL)repeatedFailure;

@end

#endif  // IOS_CHROME_BROWSER_WEB_SAD_TAB_TAB_HELPER_DELEGATE_H_
