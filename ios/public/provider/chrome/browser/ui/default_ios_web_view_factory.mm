// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/public/provider/chrome/browser/ui/default_ios_web_view_factory.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
Class g_registered_factory_class = nil;
}  // namespace

@implementation DefaultIOSWebViewFactory

+ (void)registerWebViewFactory:(Class)webViewFactoryClass {
  DCHECK([webViewFactoryClass conformsToProtocol:@protocol(IOSWebViewFactory)]);
  g_registered_factory_class = webViewFactoryClass;
}

#pragma mark -
#pragma mark IOSWebViewFactory

+ (UIWebView*)
    newExternalWebView:(IOSWebViewFactoryExternalService)externalService {
  if (g_registered_factory_class)
    return [g_registered_factory_class newExternalWebView:externalService];
  return [[UIWebView alloc] init];
}

@end
