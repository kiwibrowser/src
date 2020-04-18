// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_TEST_FAKES_CRW_FAKE_WK_NAVIGATION_RESPONSE_H_
#define IOS_WEB_TEST_FAKES_CRW_FAKE_WK_NAVIGATION_RESPONSE_H_

#import <WebKit/WebKit.h>

// Fake WKNavigationResponse class which can be used for testing.
@interface CRWFakeWKNavigationResponse : WKNavigationResponse
// Redefined WKNavigationResponse properties as readwrite.
@property(nonatomic, getter=isForMainFrame) BOOL forMainFrame;
@property(nonatomic, copy) NSURLResponse* response;
@property(nonatomic) BOOL canShowMIMEType;
@end

#endif  // IOS_WEB_TEST_FAKES_CRW_FAKE_WK_NAVIGATION_RESPONSE_H_
