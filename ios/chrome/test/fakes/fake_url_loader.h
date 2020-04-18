// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_FAKES_FAKE_URL_LOADER_H_
#define IOS_CHROME_TEST_FAKES_FAKE_URL_LOADER_H_

#import "ios/chrome/browser/ui/url_loader.h"

// URLLoader which captures argument passed into loadURLWithParams: and
// webPageOrderedOpen:referrer:inIncognito:inBackground:appendTo:.
@interface FakeURLLoader : NSObject<UrlLoader>

// These properties capture argumenents passed into protocol methods.
@property(nonatomic, readonly) const GURL& url;
@property(nonatomic, readonly) const web::Referrer& referrer;
@property(nonatomic, readonly) ui::PageTransition transition;
@property(nonatomic, readonly) BOOL rendererInitiated;
@property(nonatomic, readonly) BOOL inIncognito;

@end

#endif  // IOS_CHROME_TEST_FAKES_FAKE_URL_LOADER_H_
