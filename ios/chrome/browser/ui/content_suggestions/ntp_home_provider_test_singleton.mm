// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/ntp_home_provider_test_singleton.h"

#include <memory>

#include "components/ntp_snippets/content_suggestion.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif


@implementation ContentSuggestionsTestSingleton {
  ntp_snippets::MockContentSuggestionsProvider* _provider;
}

+ (instancetype)sharedInstance {
  static ContentSuggestionsTestSingleton* sharedInstance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

- (ntp_snippets::MockContentSuggestionsProvider*)provider {
  return _provider;
}

- (void)registerArticleProvider:
    (ntp_snippets::ContentSuggestionsService*)service {
  std::unique_ptr<ntp_snippets::MockContentSuggestionsProvider> provider =
      std::make_unique<ntp_snippets::MockContentSuggestionsProvider>(
          service, std::vector<ntp_snippets::Category>{
                       ntp_snippets::Category::FromKnownCategory(
                           ntp_snippets::KnownCategories::ARTICLES)});
  _provider = provider.get();
  service->RegisterProvider(std::move(provider));
}

@end
