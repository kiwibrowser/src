// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_NTP_HOME_PROVIDER_TEST_SINGLETON_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_NTP_HOME_PROVIDER_TEST_SINGLETON_H_

#import <UIKit/UIKit.h>

#include "components/ntp_snippets/content_suggestions_service.h"
#include "components/ntp_snippets/mock_content_suggestions_provider.h"

// Singleton allowing to register the provider in the +setup and still access it
// from inside the tests.
@interface ContentSuggestionsTestSingleton : NSObject

// Shared instance of this singleton.
+ (instancetype)sharedInstance;

// Returns the provider registered.
- (ntp_snippets::MockContentSuggestionsProvider*)provider;
// Registers a provider in the |service|.
- (void)registerArticleProvider:
    (ntp_snippets::ContentSuggestionsService*)service;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_NTP_HOME_PROVIDER_TEST_SINGLETON_H_
