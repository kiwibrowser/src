// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CHROME_WEB_VIEW_FACTORY_H_
#define IOS_CHROME_BROWSER_UI_CHROME_WEB_VIEW_FACTORY_H_

#import <Foundation/Foundation.h>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#import "ios/public/provider/chrome/browser/ui/default_ios_web_view_factory.h"

class ChromeBrowserStateIOData;

namespace ios {
class ChromeBrowserState;
}

namespace ChromeWebView {
// Shared desktop user agent used to mimic Safari on a mac.
extern NSString* const kDesktopUserAgent;

// RequestGroupID for external library, used by tests.
extern NSString* const kExternalRequestGroupID;

// Gets the external service name, used in tests.
const std::string GetExternalServiceName(
    IOSWebViewFactoryExternalService service);
}  // namespace ChromeWebView

namespace net {
class URLRequestContextGetter;
}

@interface ChromeWebViewFactory : NSObject<IOSWebViewFactory>

// Sets the browser state that will be used when creating UIWebView's for
// external libraries. Should be called before each external session.
+ (void)setBrowserStateToUseForExternal:(ios::ChromeBrowserState*)browserState;

// Should be called whenever a external session finishes.
+ (void)externalSessionFinished;

// Returns the request context from the browser state currently set for external
// library that the given external service should use for any requests that it
// makes.
+ (net::URLRequestContextGetter*)requestContextForExternalService:
    (IOSWebViewFactoryExternalService)externalService;

// Gets the partition path associated with an external service.
+ (base::FilePath)partitionPathForExternalService:
    (IOSWebViewFactoryExternalService)externalService;

// Gets the ChromeBrowserStateIOData associated with external web views.
+ (ChromeBrowserStateIOData*)ioDataForExternalWebViews;

// Clears the cookies for the |contextGetter| and the time interval between
// |deleteBegin| and |deleteEnd|.
+ (void)clearCookiesForContextGetter:
            (scoped_refptr<net::URLRequestContextGetter>)contextGetter
                            fromTime:(base::Time)deleteBegin
                              toTime:(base::Time)deleteEnd;

// Clears the cookies from the external request context for the given browser
// state and the time interval between |deleteBegin| and |deleteEnd|.
+ (void)clearExternalCookies:(IOSWebViewFactoryExternalService)externalService
                browserState:(ios::ChromeBrowserState*)browserState
                    fromTime:(base::Time)deleteBegin
                      toTime:(base::Time)deleteEnd;

// Clears the cookies in the external request contexts for the given
// browser state and the time interval between |deleteBegin| and |deleteEnd|.
+ (void)clearExternalCookies:(ios::ChromeBrowserState*)browserState
                    fromTime:(base::Time)deleteBegin
                      toTime:(base::Time)deleteEnd;

@end

#endif  // IOS_CHROME_BROWSER_UI_CHROME_WEB_VIEW_FACTORY_H_
