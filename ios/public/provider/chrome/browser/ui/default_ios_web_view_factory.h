// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_DEFAULT_IOS_WEB_VIEW_FACTORY_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_DEFAULT_IOS_WEB_VIEW_FACTORY_H_

#import <UIKit/UIKit.h>

// External services to use in |newExternalWebView:|.
typedef enum {
  SSO_AUTHENTICATION,
  NUM_SHARING_SERVICES
} IOSWebViewFactoryExternalService;

@protocol IOSWebViewFactory

// Returns a new instance of |UIWebView| that will be used within external
// libraries.
+ (UIWebView*)
    newExternalWebView:(IOSWebViewFactoryExternalService)externalService;

@end

// Factory for creating |UIWebView|'s on iOS. Returns vanilla |UIWebView|'s by
// default, with an interface by which another class implementing
// |IOSWebViewFactory| can register to have its methods called instead.
@interface DefaultIOSWebViewFactory : NSObject<IOSWebViewFactory>

+ (void)registerWebViewFactory:(Class)webViewFactoryClass;

@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_UI_DEFAULT_IOS_WEB_VIEW_FACTORY_H_
