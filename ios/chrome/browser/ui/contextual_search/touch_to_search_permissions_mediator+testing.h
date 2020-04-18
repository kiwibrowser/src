// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_TOUCH_TO_SEARCH_PERMISSIONS_MEDIATOR_TESTING_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_TOUCH_TO_SEARCH_PERMISSIONS_MEDIATOR_TESTING_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/ui/contextual_search/touch_to_search_permissions_mediator.h"

@interface TouchToSearchPermissionsMediator (Testing)
@property(nonatomic, readonly) BOOL observing;
- (BOOL)areContextualSearchQueriesSupported;
- (BOOL)isVoiceOverEnabled;
- (void)startObserving;
- (void)stopObserving;
@end

@interface MockTouchToSearchPermissionsMediator
    : TouchToSearchPermissionsMediator
// Redeclare boolean methods as simple properties that can be mocked.
@property(nonatomic, assign) BOOL canSendPageURLs;
@property(nonatomic, assign) BOOL canPreloadSearchResults;
@property(nonatomic, assign) BOOL areContextualSearchQueriesSupported;
@property(nonatomic, assign) BOOL isVoiceOverEnabled;
@property(nonatomic, assign) BOOL canExtractTapContextForAllURLs;

+ (void)setIsTouchToSearchAvailableOnDevice:(BOOL)available;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_TOUCH_TO_SEARCH_PERMISSIONS_MEDIATOR_TESTING_H_
