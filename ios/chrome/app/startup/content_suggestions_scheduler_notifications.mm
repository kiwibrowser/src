// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/app/startup/content_suggestions_scheduler_notifications.h"

#include "components/ntp_snippets/content_suggestions_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/experimental_flags.h"
#include "ios/chrome/browser/ntp_snippets/ios_chrome_content_suggestions_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ContentSuggestionsSchedulerNotifications

+ (void)notifyColdStart:(ios::ChromeBrowserState*)browserState {
  ntp_snippets::ContentSuggestionsService* contentSuggestionsService =
      IOSChromeContentSuggestionsServiceFactory::GetForBrowserState(
          browserState);
  contentSuggestionsService->remote_suggestions_scheduler()
      ->OnBrowserColdStart();
}

+ (void)notifyForeground:(ios::ChromeBrowserState*)browserState {
  ntp_snippets::ContentSuggestionsService* contentSuggestionsService =
      IOSChromeContentSuggestionsServiceFactory::GetForBrowserState(
          browserState);
  contentSuggestionsService->remote_suggestions_scheduler()
      ->OnBrowserForegrounded();
}

@end
