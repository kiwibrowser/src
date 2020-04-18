// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/fakes/fake_url_loader.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FakeURLLoader () {
  GURL _url;
  web::Referrer _referrer;
}

@property(nonatomic) ui::PageTransition transition;
@property(nonatomic) BOOL rendererInitiated;
@property(nonatomic) BOOL inIncognito;
@end

@implementation FakeURLLoader

@synthesize transition = _transition;
@synthesize rendererInitiated = _rendererInitiated;
@synthesize inIncognito = _inIncognito;

- (void)loadURLWithParams:(const web::NavigationManager::WebLoadParams&)params {
  _url = params.url;
  _referrer = params.referrer;
  self.transition = params.transition_type;
  self.rendererInitiated = params.is_renderer_initiated;
}

- (void)webPageOrderedOpen:(const GURL&)url
                  referrer:(const web::Referrer&)referrer
              inBackground:(BOOL)inBackground
                  appendTo:(OpenPosition)appendTo {
}

- (void)webPageOrderedOpen:(const GURL&)url
                  referrer:(const web::Referrer&)referrer
               inIncognito:(BOOL)inIncognito
              inBackground:(BOOL)inBackground
                  appendTo:(OpenPosition)appendTo {
  _url = url;
  _referrer = referrer;
  self.inIncognito = inIncognito;
}

- (void)loadSessionTab:(const sessions::SessionTab*)sessionTab {
}

- (void)loadJavaScriptFromLocationBar:(NSString*)script {
}

- (const GURL&)url {
  return _url;
}

- (const web::Referrer&)referrer {
  return _referrer;
}

@end
