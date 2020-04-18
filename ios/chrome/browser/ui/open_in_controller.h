// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OPEN_IN_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_OPEN_IN_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include <memory>

#include "base/memory/ref_counted.h"
#import "ios/chrome/browser/ui/open_in_toolbar.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

@class CRWWebController;

// Class used to handle opening files in other applications.
@interface OpenInController : NSObject<UIGestureRecognizerDelegate,
                                       UIDocumentInteractionControllerDelegate>
// Designated initializer.
- (id)initWithRequestContext:(net::URLRequestContextGetter*)requestContext
               webController:(CRWWebController*)webController;

// Removes the |openInToolbar_| from the |webController_|'s view and resets the
// variables specific to the loaded document.
- (void)disable;

// Disconnects the controller from its CRWWebController. Should be called when
// the CRWWebController is being torn down.
- (void)detachFromWebController;

// Adds the |openInToolbar_| to the |webController_|'s view and sets the url and
// the filename for the currently loaded document.
- (void)enableWithDocumentURL:(const GURL&)documentURL
            suggestedFilename:(NSString*)suggestedFilename;
@end

#endif  // IOS_CHROME_BROWSER_UI_OPEN_IN_CONTROLLER_H_
